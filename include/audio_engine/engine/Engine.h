#pragma once

#include <thread>
#include <atomic>

#include "audio_engine/buffer/RingBuffer.h"
#include "audio_engine/stream/AudioStream.h"
#include "audio_engine/audio/AudioFrame.h"
#include "audio_engine/output/CoreAudioOutput.h"

namespace audio_engine::engine {

/*
 * Engine coordinates:
 * - Producer thread (pulls from AudioStream)
 * - Lock-free ring buffer
 * - Core Audio output (consumer via callback)
 *
 * The Engine itself does NOT render audio.
 * Rendering is done by Core Audio on a real-time thread.
 */
class Engine {
public:
    explicit Engine(stream::AudioStream& stream,
                    size_t buffer_capacity = 8)
        : stream_(stream),
          buffer_(buffer_capacity),
          output_(buffer_)
    {}

    ~Engine() {
        stop();
    }

    void start() {
        if (running_.exchange(true)) {
            return; // already running
        }

        output_.start();
        producer_ = std::thread(&Engine::producer_loop, this);
    }

    void stop() {
        if (!running_.exchange(false)) {
            return; // already stopped
        }

        if (producer_.joinable()) {
            producer_.join();
        }

        output_.stop();
    }

private:
    void producer_loop() {
        while (running_.load(std::memory_order_relaxed)) {
            auto frame = stream_.next_frame();
            if (!frame) {
                // No audio available yet
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                continue;
            }

            // Push frame into ring buffer (backpressure-aware)
            while (running_.load(std::memory_order_relaxed) &&
                   !buffer_.push(*frame)) {
                // Buffer full → yield briefly
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

private:
    stream::AudioStream& stream_;
    buffer::RingBuffer<audio::AudioFrame> buffer_;
    output::CoreAudioOutput output_;

    std::atomic<bool> running_{false};
    std::thread producer_;
};

} // namespace audio_engine::engine
