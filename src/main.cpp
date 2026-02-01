#include <iostream>
#include <thread>
#include <chrono>

#include "audio_engine/buffer/RingBuffer.h"
#include "audio_engine/audio/AudioFrame.h"

using audio_engine::buffer::RingBuffer;
using audio_engine::audio::AudioFrame;

int main() {
    RingBuffer<AudioFrame> buffer(4);

    std::thread producer([&] {
        for (int i = 0; i < 10; ++i) {
            AudioFrame frame(48000, 2, 256);

            // Fake audio data
            for (auto& sample : frame.samples) {
                sample = static_cast<float>(i);
            }

            while (!buffer.push(frame)) {
                // buffer full
            }

            std::cout << "[producer] pushed frame " << i << "\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    std::thread consumer([&] {
        AudioFrame frame;
        for (int i = 0; i < 10; ++i) {
            while (!buffer.pop(frame)) {
                // buffer empty
            }

            std::cout << "  [consumer] popped frame with "
                      << frame.frames << " frames\n";
        }
    });

    producer.join();
    consumer.join();

    std::cout << "Phase 3 test complete\n";
    return 0;
}
