#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <optional>
#include <vector>

#include "audio_engine/stream/AudioStream.h"
#include "audio_engine/network/UdpAudioPacket.h"

namespace audio_engine::network {

class UdpReceiverStream : public audio_engine::stream::AudioStream {
public:
    explicit UdpReceiverStream(uint16_t port) {
        sock_ = socket(AF_INET, SOCK_DGRAM, 0);

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
        if (sock_ >= 0)
            close(sock_);
    }

    std::optional<audio_engine::audio::AudioFrame>
    next_frame() override {
        std::vector<uint8_t> buffer(65536);

        ssize_t bytes =
            recv(sock_, buffer.data(), buffer.size(), MSG_DONTWAIT);

        if (bytes <= 0)
            return std::nullopt;

        auto* header =
            reinterpret_cast<UdpAudioHeader*>(buffer.data());

        audio_engine::audio::AudioFrame frame(
            header->sample_rate,
            header->channels,
            header->frames
        );

        std::memcpy(frame.samples.data(),
                    buffer.data() + sizeof(UdpAudioHeader),
                    frame.samples.size() * sizeof(float));

        return frame;
    }

private:
    int sock_{-1};
};

} // namespace audio_engine::network
