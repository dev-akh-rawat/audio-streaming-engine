#pragma once

#include <cmath>

#include "audio_engine/stream/AudioStream.h"

namespace audio_engine::stream {

class SineWaveStream : public AudioStream {
public:
    SineWaveStream(uint32_t sample_rate,
                   uint16_t channels,
                   uint16_t frames_per_buffer)
        : sr_(sample_rate),
          ch_(channels),
          frames_(frames_per_buffer)
    {}

    std::optional<audio_engine::audio::AudioFrame>
    next_frame() override {
        audio_engine::audio::AudioFrame frame(sr_, ch_, frames_);

        for (size_t i = 0; i < frame.samples.size(); ++i) {
            frame.samples[i] = std::sin(phase_);
            phase_ += 0.01f;
        }

        return frame;
    }

private:
    uint32_t sr_;
    uint16_t ch_;
    uint16_t frames_;
    float phase_ = 0.0f;
};

} // namespace audio_engine::stream
