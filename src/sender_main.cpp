#include <thread>
#include <chrono>

#include "audio_engine/stream/SineWaveStream.h"
#include "audio_engine/network/UdpSender.h"

using audio_engine::stream::SineWaveStream;
using audio_engine::network::UdpSender;

int main() {
    SineWaveStream stream(48000, 2, 256);
    UdpSender sender("127.0.0.1", 5000);

    while (true) {
        auto frame = stream.next_frame();
        if (frame) {
            sender.send_frame(*frame);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    return 0;
}
