#pragma once

#include <cstdint>

namespace audio_engine::network {

/*
 * Flags for UDP audio packets
 */
enum AudioPacketFlags : uint32_t {
    AUDIO_FLAG_NONE = 0,
    AUDIO_FLAG_EOS  = 1 << 0   // End-of-stream marker
};

/*
 * Fixed-size header for UDP audio packets.
 * Payload follows immediately after header.
 */
struct UdpAudioHeader {
    uint32_t sequence;     // Packet sequence number
    uint32_t sample_rate;  // Hz
    uint16_t channels;     // e.g. 2
    uint16_t frames;       // Frames in payload (0 for EOS)
    uint64_t pts;          // Presentation timestamp (in samples)
    uint32_t flags;        // AudioPacketFlags
};

} // namespace audio_engine::network
