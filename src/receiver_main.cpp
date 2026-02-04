#include <thread>
#include <chrono>
#include <iostream>

#include "audio_engine/engine/Engine.h"
#include "audio_engine/network/UdpReceiverStream.h"
#include "audio_engine/network/Metrics.h"

/*int main() {
    using namespace audio_engine;

    network::Metrics metrics;

    engine::Engine engine(metrics);

    network::UdpReceiverStream receiver(
        5000,
        metrics,
        engine.buffer()
    );

    receiver.start();
    engine.start();

    std::thread stats([&] {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout
                << "Packets recv: " << metrics.packets_received.load()
                << " | Dropped: " << metrics.packets_dropped.load()
                << " | Underruns: " << metrics.underruns.load()
                << " | Jitter depth: " << metrics.jitter_depth.load()
                << std::endl;
        }
    });

    std::this_thread::sleep_for(std::chrono::seconds(20));

    receiver.stop();
    engine.stop();
    stats.detach();

    return 0;
}*/
#include "audio_engine/stream/WavFileStream.h"
#include "audio_engine/engine/Engine.h"
#include "audio_engine/network/Metrics.h"

int main() {
    using namespace audio_engine;

    network::Metrics metrics;

    stream::WavFileStream music(
        "/Users/akarsh_rawat/dev_projects/audio-streaming-engine/Mozart_from_Piano_Sonata_K310_first_movement.wav"
    );

    engine::Engine engine(metrics);
    engine.start();

    // Feed WAV directly into engine ring buffer
    while (auto frame = music.next_frame()) {
        while (!engine.buffer().push(*frame)) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(1));
        }
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));
    engine.stop();
}

