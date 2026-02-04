#pragma once

#include <cstdint>

namespace audio_engine::network {

/*
 * Fixed-size header for UDP audio packets.
 * Payload follows immediately after header.
 */
struct UdpAudioHeader {
    uint32_t sample_rate;
    uint16_t channels;
    uint16_t frames;
    uint32_t sequence;
};

} // namespace audio_engine::network
