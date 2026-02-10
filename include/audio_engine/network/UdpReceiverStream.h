#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <thread>
#include <vector>
#include <cstring>
#include <chrono>
#include <optional>

#include "audio_engine/network/UdpAudioPacket.h"
#include "audio_engine/network/JitterBuffer.h"
#include "audio_engine/network/Metrics.h"
#include "audio_engine/buffer/RingBuffer.h"
#include "audio_engine/audio/AudioFrame.h"
#include "audio_engine/engine/Engine.h"

namespace audio_engine::network {

/*
 * UdpReceiverStream
 *
 * - Network thread receives UDP packets
 * - JitterBuffer absorbs jitter & reorders packets
 * - Drain thread enforces media timing (PTS → wall clock)
 * - RingBuffer hands off to CoreAudio safely
 */
class UdpReceiverStream {
public:
    UdpReceiverStream(
        uint16_t port,
        Metrics& metrics,
        engine::Engine& engine,
        buffer::RingBuffer<audio::AudioFrame>& ring)
        : jitter_(12, metrics),
          engine_(engine),
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
        running_.store(true, std::memory_order_relaxed);
        recv_thread_  = std::thread(&UdpReceiverStream::recv_loop, this);
        drain_thread_ = std::thread(&UdpReceiverStream::drain_loop, this);
    }

    void stop() {
        running_.store(false, std::memory_order_relaxed);
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

            // 🔑 EOS detection
            if (header.flags & AUDIO_FLAG_EOS) {
                stream_ended_.store(true, std::memory_order_release);
                continue;
            }

            audio::AudioFrame frame(
                header.sample_rate,
                header.channels,
                header.frames,
                header.pts
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

        static constexpr size_t kPrerollFrames = 30;

        std::optional<uint64_t> first_pts;
        clock::time_point stream_start_time;

        bool stream_started = false;
        bool preroll_done = false;
        bool engine_eos_signaled = false;

        while (running_.load(std::memory_order_relaxed)) {

            // 🔑 EOS propagation after jitter fully drained
            if (stream_ended_.load(std::memory_order_acquire) &&
                jitter_.empty() &&
                !engine_eos_signaled) {

                engine_.mark_stream_ended();
                engine_eos_signaled = true;
            }

            // ---------------- PRE-ROLL GATE ----------------
            if (!preroll_done) {
                if (jitter_.depth() < kPrerollFrames) {
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(1));
                    continue;
                }
                preroll_done = true;
            }
            // ------------------------------------------------

            auto frame = jitter_.pop();
            if (!frame) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(1));
                continue;
            }

            // Initialize media timeline ONCE (NO clock shift!)
            if (!first_pts.has_value()) {
                first_pts = frame->pts;
                stream_start_time = clock::now();
            }

            // PTS → wall-clock
            const uint64_t pts_offset =
                frame->pts - *first_pts;

            const double seconds_from_start =
                double(pts_offset) / double(frame->sample_rate);

            const auto target_time =
                stream_start_time +
                std::chrono::duration_cast<clock::duration>(
                    std::chrono::duration<double>(seconds_from_start));

            auto now = clock::now();
            if (now + std::chrono::milliseconds(1) < target_time) {
                std::this_thread::sleep_until(target_time);
            }
            // else: already late → push immediately

            // Push to ring buffer (RT-safe handoff)
            while (running_.load(std::memory_order_relaxed) &&
                   !ring_.push(*frame)) {
                std::this_thread::sleep_for(
                    std::chrono::microseconds(200));
            }

            // 🔑 Signal stream start ONCE
            if (!stream_started) {
                engine_.mark_stream_started();
                stream_started = true;
            }
        }
    }

private:
    int sock_{-1};

    JitterBuffer jitter_;
    engine::Engine& engine_;
    buffer::RingBuffer<audio::AudioFrame>& ring_;

    std::atomic<bool> running_{false};
    std::thread recv_thread_;
    std::thread drain_thread_;
    std::atomic<bool> stream_ended_{false};
};

} // namespace audio_engine::network
