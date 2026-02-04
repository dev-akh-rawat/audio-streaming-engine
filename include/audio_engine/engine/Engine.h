#pragma once

#include <thread>
#include <atomic>

#include "audio_engine/buffer/RingBuffer.h"
#include "audio_engine/audio/AudioFrame.h"
#include "audio_engine/output/CoreAudioOutput.h"
#include "audio_engine/network/Metrics.h"

namespace audio_engine::engine {

/*
 * Engine
 *
 * - Owns RingBuffer
 * - Starts Core Audio
 * - NO networking
 * - NO jitter logic
 */
class Engine {
public:
    Engine(network::Metrics& metrics,
           size_t buffer_capacity = 8)
        : buffer_(buffer_capacity),
          output_(buffer_, metrics)
    {}

    buffer::RingBuffer<audio::AudioFrame>& buffer() {
        return buffer_;
    }

    void start() {
        output_.start();
    }

    void stop() {
        output_.stop();
    }

private:
    buffer::RingBuffer<audio::AudioFrame> buffer_;
    output::CoreAudioOutput output_;
};

} // namespace audio_engine::engine
