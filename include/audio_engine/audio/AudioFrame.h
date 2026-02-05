#pragma once

#include <vector>
#include <cstdint>

namespace audio_engine::audio {

/*
 * AudioFrame represents a block of audio samples.
 * This is the atomic unit exchanged between producer and consumer.
 */
struct AudioFrame {
    uint32_t sample_rate{0};
    uint16_t channels{0};
    uint16_t frames{0};
    uint64_t pts{0};
    std::vector<float> samples;

    // ✅ Default constructor (required)
    AudioFrame() = default;

    // Media constructor
    AudioFrame(uint32_t sr,
               uint16_t ch,
               uint16_t fr,
               uint64_t p = 0)
        : sample_rate(sr),
          channels(ch),
          frames(fr),
          pts(p),
          samples(fr * ch) {}
};


} // namespace audio_engine::audio
