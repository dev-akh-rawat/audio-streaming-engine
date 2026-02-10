#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <cstdint>

#include "audio_engine/network/UdpAudioPacket.h"
#include "audio_engine/audio/AudioFrame.h"

namespace audio_engine::network {

/*
 * UdpSender
 *
 * - Sends audio frames over UDP
 * - Also sends explicit End-Of-Stream (EOS) packet
 */
class UdpSender {
public:
    UdpSender(const char* ip, uint16_t port) {
        sock_ = socket(AF_INET, SOCK_DGRAM, 0);

        std::memset(&addr_, 0, sizeof(addr_));
        addr_.sin_family = AF_INET;
        addr_.sin_port   = htons(port);
        inet_pton(AF_INET, ip, &addr_.sin_addr);
    }

    ~UdpSender() {
        if (sock_ >= 0)
            close(sock_);
    }

    // ---------------- Audio frame ----------------
    void send_frame(const audio::AudioFrame& frame,
                    uint32_t sequence,
                    uint64_t pts) {

        UdpAudioHeader header{};
        header.sequence    = sequence;
        header.sample_rate = frame.sample_rate;
        header.channels    = frame.channels;
        header.frames      = frame.frames;
        header.pts         = pts;
        header.flags       = AUDIO_FLAG_NONE;

        const size_t payload_bytes =
            frame.samples.size() * sizeof(float);

        std::vector<uint8_t> packet(
            sizeof(UdpAudioHeader) + payload_bytes);

        std::memcpy(packet.data(),
                    &header,
                    sizeof(UdpAudioHeader));

        std::memcpy(packet.data() + sizeof(UdpAudioHeader),
                    frame.samples.data(),
                    payload_bytes);

        sendto(sock_,
               packet.data(),
               packet.size(),
               0,
               reinterpret_cast<sockaddr*>(&addr_),
               sizeof(addr_));
    }

    // ---------------- End of stream ----------------
    void send_eos(uint32_t sequence, uint64_t pts) {
        UdpAudioHeader header{};
        header.sequence    = sequence;
        header.sample_rate = 0;
        header.channels    = 0;
        header.frames      = 0;              // no audio payload
        header.pts         = pts;
        header.flags       = AUDIO_FLAG_EOS;

        sendto(sock_,
               &header,
               sizeof(header),
               0,
               reinterpret_cast<sockaddr*>(&addr_),
               sizeof(addr_));
    }

private:
    int sock_{-1};
    sockaddr_in addr_{};
};

} // namespace audio_engine::network
