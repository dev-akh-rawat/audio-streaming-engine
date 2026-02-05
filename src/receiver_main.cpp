#include <thread>
#include <chrono>
#include <iostream>

#include "audio_engine/engine/Engine.h"
#include "audio_engine/network/UdpReceiverStream.h"
#include "audio_engine/network/Metrics.h"

int main() {
    using namespace audio_engine;

    network::Metrics metrics;

    engine::Engine engine(metrics);

    network::UdpReceiverStream receiver(
        5000,          // listen port
        metrics,
        engine,        // ✅ ADD THIS
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
}
