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
          output_(buffer_, metrics, stream_started_)
    {}

    buffer::RingBuffer<audio::AudioFrame>& buffer() {
        return buffer_;
    }

    void start() {
        stream_started_.store(false, std::memory_order_relaxed);
        output_.start();
    }

    void stop() {
        output_.stop();
        stream_started_.store(false, std::memory_order_relaxed);
    }

    // 🔑 Called by drain thread once first frame is scheduled
    void mark_stream_started() {
        stream_started_.store(true, std::memory_order_release);
    }

private:
    buffer::RingBuffer<audio::AudioFrame> buffer_;
    std::atomic<bool> stream_started_{false};   // ✅ ADD THIS
    output::CoreAudioOutput output_;
};

} // namespace audio_engine::engine
