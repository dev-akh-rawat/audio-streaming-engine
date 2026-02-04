#include <thread>
#include <chrono>

#include "audio_engine/network/UdpReceiverStream.h"
#include "audio_engine/engine/Engine.h"

int main() {
    audio_engine::network::UdpReceiverStream stream(5000);
    audio_engine::engine::Engine engine(stream);

    engine.start();
    std::this_thread::sleep_for(std::chrono::seconds(20));
    engine.stop();

    return 0;
}
