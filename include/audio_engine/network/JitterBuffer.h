#pragma once

#include <map>
#include <optional>
#include <cstdint>

#include "audio_engine/audio/AudioFrame.h"
#include "audio_engine/network/Metrics.h"

namespace audio_engine::network {
static constexpr size_t kMaxDepth = 50;

/*
 * JitterBuffer:
 * - Reorders UDP packets
 * - Absorbs timing jitter
 * - Detects packet loss
 *
 * NOT real-time critical.
 */
class JitterBuffer {
public:
    JitterBuffer(size_t target_depth, Metrics& metrics)
            : target_depth_(target_depth),
            metrics_(metrics)
        {}

        void push(uint32_t sequence,
            audio_engine::audio::AudioFrame&& frame) {

        buffer_.emplace(sequence, std::move(frame));
        metrics_.packets_received.fetch_add(1,
                                            std::memory_order_relaxed);

        // ---- SAFETY CAP (IMPORTANT) ----
        if (buffer_.size() > kMaxDepth) {
            // Drop the oldest packet (lowest sequence)
            buffer_.erase(buffer_.begin());
            metrics_.packets_dropped.fetch_add(1,
                                            std::memory_order_relaxed);
        }

        metrics_.jitter_depth.store(buffer_.size(),
                                    std::memory_order_relaxed);
    }


    std::optional<audio_engine::audio::AudioFrame> pop() {
        if (buffer_.size() < target_depth_) {
            return std::nullopt;
        }

        auto it = buffer_.begin();

        if (expected_sequence_ == 0) {
            expected_sequence_ = it->first;
        }

        if (it->first != expected_sequence_) {
            // Packet loss detected
            metrics_.packets_dropped.fetch_add(1,
                                               std::memory_order_relaxed);
            ++expected_sequence_;
            return std::nullopt;
        }

        auto frame = std::move(it->second);
        buffer_.erase(it);
        ++expected_sequence_;

        metrics_.jitter_depth.store(buffer_.size(),
                                    std::memory_order_relaxed);
        return frame;
    }

private:
    size_t target_depth_;
    uint32_t expected_sequence_{0};

    std::map<uint32_t, audio_engine::audio::AudioFrame> buffer_;
    Metrics& metrics_;
};

} // namespace audio_engine::network
