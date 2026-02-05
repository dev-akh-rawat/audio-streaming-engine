#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <cstring>
#include <time.h>
#include "audio_engine/audio/AudioFrame.h"
#include "audio_engine/network/UdpAudioPacket.h"

namespace audio_engine::network {

class UdpSender {
public:
    UdpSender(const char* ip, uint16_t port) {
        sock_ = socket(AF_INET, SOCK_DGRAM, 0);

        addr_.sin_family = AF_INET;
        addr_.sin_port   = htons(port);
        inet_pton(AF_INET, ip, &addr_.sin_addr);
    }

    ~UdpSender() {
        if (sock_ >= 0)
            close(sock_);
    }

    void send_frame(const audio::AudioFrame& frame,
                uint32_t sequence,
                uint64_t pts){
        UdpAudioHeader header{};
        header.sequence    = sequence;
        header.sample_rate = frame.sample_rate;
        header.channels    = frame.channels;
        header.frames      = frame.frames;
        header.pts         = pts;


        const size_t payload_bytes =
            frame.samples.size() * sizeof(float);

        std::vector<uint8_t> packet(
            sizeof(UdpAudioHeader) + payload_bytes
        );

        std::memcpy(packet.data(), &header, sizeof(header));
        std::memcpy(packet.data() + sizeof(header),
                    frame.samples.data(),
                    payload_bytes);

        sendto(sock_,
               packet.data(),
               packet.size(),
               0,
               reinterpret_cast<sockaddr*>(&addr_),
               sizeof(addr_));
    }

    uint64_t now_ns() {
        timespec ts{};
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return uint64_t(ts.tv_sec) * 1'000'000'000ULL + ts.tv_nsec;
    }

private:
    int sock_{-1};
    sockaddr_in addr_{};
};

} // namespace audio_engine::network
