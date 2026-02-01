#pragma once

#include <vector>
#include <cstdint>

namespace audio_engine::audio {

/*
 * AudioFrame represents a block of audio samples.
 * This is the atomic unit exchanged between producer and consumer.
 */
struct AudioFrame {
    uint32_t sample_rate = 0;     // e.g. 44100, 48000
    uint16_t channels    = 0;     // mono = 1, stereo = 2
    uint16_t frames      = 0;     // number of samples per channel

    // Interleaved samples: LRLRLR...
    std::vector<float> samples;

    AudioFrame() = default;

    AudioFrame(uint32_t sr, uint16_t ch, uint16_t f)
        : sample_rate(sr),
          channels(ch),
          frames(f),
          samples(ch * f, 0.0f)
    {}
};

} // namespace audio_engine::audio
