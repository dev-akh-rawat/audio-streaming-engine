#pragma once

#include <optional>

#include "audio_engine/audio/AudioFrame.h"

namespace audio_engine::stream {

/*
 * AudioStream is a producer of AudioFrames.
 * Implementations may read from files, network, or generators.
 */
class AudioStream {
public:
    virtual ~AudioStream() = default;

    // Produce the next audio frame.
    // Returns nullopt when no data is available.
    virtual std::optional<audio_engine::audio::AudioFrame>
    next_frame() = 0;
};

} // namespace audio_engine::stream
