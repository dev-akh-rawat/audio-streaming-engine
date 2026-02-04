#pragma once

#include <atomic>
#include <cstdint>

namespace audio_engine::network {

struct Metrics {
    std::atomic<uint64_t> packets_received{0};
    std::atomic<uint64_t> packets_dropped{0};
    std::atomic<uint64_t> packets_reordered{0};

    std::atomic<uint64_t> frames_rendered{0};
    std::atomic<uint64_t> underruns{0};

    std::atomic<uint32_t> jitter_depth{0};
};

} // namespace audio_engine::network
