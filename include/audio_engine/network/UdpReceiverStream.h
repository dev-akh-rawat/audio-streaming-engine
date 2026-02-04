#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <thread>
#include <vector>
#include <cstring>
#include <chrono>

#include "audio_engine/network/UdpAudioPacket.h"
#include "audio_engine/network/JitterBuffer.h"
#include "audio_engine/network/Metrics.h"
#include "audio_engine/buffer/RingBuffer.h"
#include "audio_engine/audio/AudioFrame.h"

namespace audio_engine::network {

/*
 * UdpReceiverStream
 *
 * - Network thread receives UDP packets
 * - JitterBuffer absorbs jitter
 * - Drain thread moves frames into RingBuffer
 *
 * Audio engine NEVER touches sockets or jitter buffer.
 */
class UdpReceiverStream {
public:
    UdpReceiverStream(
        uint16_t port,
        Metrics& metrics,
        buffer::RingBuffer<audio::AudioFrame>& ring)
        : jitter_(3, metrics),
          ring_(ring)
    {
        sock_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock_ < 0) {
            perror("socket failed");
            return;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(sock_,
                 reinterpret_cast<sockaddr*>(&addr),
                 sizeof(addr)) < 0) {
            perror("bind failed");
            close(sock_);
            sock_ = -1;
        }
    }

    ~UdpReceiverStream() {
        stop();
        if (sock_ >= 0)
            close(sock_);
    }

    void start() {
        running_.store(true);
        recv_thread_  = std::thread(&UdpReceiverStream::recv_loop, this);
        drain_thread_ = std::thread(&UdpReceiverStream::drain_loop, this);
    }

    void stop() {
        running_.store(false);
        if (recv_thread_.joinable())
            recv_thread_.join();
        if (drain_thread_.joinable())
            drain_thread_.join();
    }

private:
    /* ---------------- Network receive thread ---------------- */
    void recv_loop() {
        std::vector<uint8_t> buffer(65536);

        while (running_.load(std::memory_order_relaxed)) {
            ssize_t bytes =
                recv(sock_, buffer.data(), buffer.size(), MSG_DONTWAIT);

            if (bytes <= 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            if (bytes < static_cast<ssize_t>(sizeof(UdpAudioHeader)))
                continue;

            UdpAudioHeader header{};
            std::memcpy(&header, buffer.data(), sizeof(header));

            audio::AudioFrame frame(
                header.sample_rate,
                header.channels,
                header.frames
            );

            const size_t payload_bytes =
                frame.samples.size() * sizeof(float);

            if (bytes < static_cast<ssize_t>(
                    sizeof(UdpAudioHeader) + payload_bytes))
                continue;

            std::memcpy(frame.samples.data(),
                        buffer.data() + sizeof(UdpAudioHeader),
                        payload_bytes);

            jitter_.push(header.sequence, std::move(frame));
        }
    }

    /* ---------------- Jitter drain thread ---------------- */
    void drain_loop() {
        using clock = std::chrono::steady_clock;

        auto next_wakeup = clock::now();

        while (running_.load(std::memory_order_relaxed)) {
            auto frame = jitter_.pop();
            if (!frame) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            // Push to ring buffer
            while (running_.load(std::memory_order_relaxed) &&
                !ring_.push(*frame)) {
                std::this_thread::sleep_for(
                    std::chrono::microseconds(200));
            }

            // Derive timing from media clock
            const double seconds =
                double(frame->frames) / double(frame->sample_rate);

            const auto frame_period =
                std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                    std::chrono::duration<double>(seconds));

            next_wakeup += frame_period;
            std::this_thread::sleep_until(next_wakeup);
        }
    }



private:
    int sock_{-1};

    JitterBuffer jitter_;
    buffer::RingBuffer<audio::AudioFrame>& ring_;

    std::atomic<bool> running_{false};
    std::thread recv_thread_;
    std::thread drain_thread_;
};

} // namespace audio_engine::network
