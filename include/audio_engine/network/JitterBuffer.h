#pragma once

#include <array>
#include <optional>
#include <cstdint>

#include "audio_engine/audio/AudioFrame.h"
#include "audio_engine/network/Metrics.h"

namespace audio_engine::network {

/*
 * Ordered SPSC Jitter Buffer (Option B, Variant 2 — FIXED)
 *
 * - Sequence-based reordering ONLY
 * - No timing logic
 * - Window slides forward safely
 */
class JitterBuffer {
public:
    static constexpr size_t kWindowSize = 30;

    explicit JitterBuffer(size_t /*unused*/, Metrics& metrics)
        : metrics_(metrics)
    {}

    // ---------------- Producer ----------------
    void push(uint32_t sequence, audio::AudioFrame&& frame) {
        metrics_.packets_received.fetch_add(
            1, std::memory_order_relaxed);

        if (!initialized_) {
            expected_sequence_ = sequence;
            initialized_ = true;
        }

        // Too old → drop
        if (sequence < expected_sequence_) {
            metrics_.packets_dropped.fetch_add(
                1, std::memory_order_relaxed);
            return;
        }

        // Slide window forward if sender runs ahead
        while (sequence >= expected_sequence_ + kWindowSize) {
            const size_t evict = expected_sequence_ % kWindowSize;
            if (slots_[evict].has_value()) {
                slots_[evict].reset();
                occupancy_--;
                metrics_.packets_dropped.fetch_add(
                    1, std::memory_order_relaxed);
            }
            expected_sequence_++;
        }

        const size_t slot = sequence % kWindowSize;

        if (!slots_[slot].has_value()) {
            slots_[slot] = std::move(frame);
            occupancy_++;
        }

        metrics_.jitter_depth.store(
            occupancy_, std::memory_order_relaxed);
    }

    // ---------------- Consumer ----------------
    std::optional<audio::AudioFrame> pop() {
        if (!initialized_ || occupancy_ == 0) {
            return std::nullopt;
        }

        const size_t slot = expected_sequence_ % kWindowSize;

        if (!slots_[slot].has_value()) {
            return std::nullopt;
        }

        auto frame = std::move(slots_[slot]);
        slots_[slot].reset();
        expected_sequence_++;
        occupancy_--;

        metrics_.jitter_depth.store(
            occupancy_, std::memory_order_relaxed);

        return frame;
    }

    // ---------------- Introspection ----------------
    size_t depth() const {
        return occupancy_;
    }

    bool empty() const {
        return occupancy_ == 0;
    }

private:
    Metrics& metrics_;

    std::array<std::optional<audio::AudioFrame>, kWindowSize> slots_{};

    uint32_t expected_sequence_{0};
    bool initialized_{false};

    size_t occupancy_{0};
};

} // namespace audio_engine::network
