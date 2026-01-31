#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "Audio Streaming Engine starting...\n";

    // Simulate an audio thread
    std::thread audio_thread([] {
        std::cout << "[audio] thread running\n";

        // Simulate real-time work
        for (int i = 0; i < 5; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::cout << "[audio] processing frame " << i << "\n";
        }

        std::cout << "[audio] thread exiting\n";
    });

    audio_thread.join();

    std::cout << "Audio Streaming Engine stopped.\n";
    return 0;
}
