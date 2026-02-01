#pragma once

#include <atomic>
#include <cstddef>
#include <vector>
#include <cassert>

namespace audio_engine::buffer {
/*
 * Lock-free Single Producer Single Consumer (SPSC) ring buffer.
 *
 * Properties:
 * - One producer thread
 * - One consumer thread
 * - Wait-free for producer and consumer
 * - No dynamic allocation in push/pop
 */
template <typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t capacity)
        : capacity_(capacity + 1),  // one slot is sacrificed to distinguish full vs empty
          buffer_(capacity_)
    {
        assert(capacity > 0);
    }

    // Non-copyable
    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    bool push(const T& item) {
        const size_t write = write_.load(std::memory_order_relaxed);
        const size_t next  = increment(write);

        if (next == read_.load(std::memory_order_acquire)) {
            return false; // buffer full
        }

        buffer_[write] = item;
        write_.store(next, std::memory_order_release);
        return true;
    }

    bool pop(T& item) {
        const size_t read = read_.load(std::memory_order_relaxed);

        if (read == write_.load(std::memory_order_acquire)) {
            return false; // buffer empty
        }

        item = buffer_[read];
        read_.store(increment(read), std::memory_order_release);
        return true;
    }

    bool empty() const {
        return read_.load(std::memory_order_acquire) ==
               write_.load(std::memory_order_acquire);
    }

    bool full() const {
        const size_t next = increment(write_.load(std::memory_order_acquire));
        return next == read_.load(std::memory_order_acquire);
    }

    size_t capacity() const {
        return capacity_ - 1;
    }

private:
    size_t increment(size_t idx) const {
        return (idx + 1) % capacity_;
    }

    const size_t capacity_;
    std::vector<T> buffer_;

    alignas(64) std::atomic<size_t> write_{0};
    alignas(64) std::atomic<size_t> read_{0};
};

}