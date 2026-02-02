#include <thread>
#include <chrono>

#include "audio_engine/engine/Engine.h"
#include "audio_engine/stream/SineWaveStream.h"

int main() {
    audio_engine::stream::SineWaveStream stream(48000, 2, 256);
    audio_engine::engine::Engine engine(stream);

    engine.start();
    std::this_thread::sleep_for(std::chrono::seconds(5));
    engine.stop();

    return 0;
}
