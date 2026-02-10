#pragma once

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
 * - Owns stream lifecycle (started / ended)
 * - Starts / stops Core Audio
 *
 * NO networking
 * NO jitter logic
 */
class Engine {
public:
    Engine(network::Metrics& metrics,
           size_t buffer_capacity = 64)
        : buffer_(buffer_capacity),
          stream_started_(false),
          stream_ended_(false),
          output_(buffer_, metrics,
                  stream_started_,
                  stream_ended_)   //
    {}

    buffer::RingBuffer<audio::AudioFrame>& buffer() {
        return buffer_;
    }

    void start() {
        stream_started_.store(false, std::memory_order_relaxed);
        stream_ended_.store(false, std::memory_order_relaxed);
        output_.start();
    }

    void stop() {
        output_.stop();
        stream_started_.store(false, std::memory_order_relaxed);
        stream_ended_.store(false, std::memory_order_relaxed);
    }

    // Called by drain thread once first frame is scheduled
    void mark_stream_started() {
        stream_started_.store(true, std::memory_order_release);
    }

    // Called by drain thread after EOS + jitter drained
    void mark_stream_ended() {
        stream_ended_.store(true, std::memory_order_release);
    }

    bool stream_started() const {
        return stream_started_.load(std::memory_order_acquire);
    }

    bool stream_ended() const {
        return stream_ended_.load(std::memory_order_acquire);
    }

private:
    buffer::RingBuffer<audio::AudioFrame> buffer_;
    std::atomic<bool> stream_started_;
    std::atomic<bool> stream_ended_;
    output::CoreAudioOutput output_;
};

} // namespace audio_engine::engine
