#include <chrono>
#include <thread>

#include "audio_engine/network/UdpSender.h"
#include "audio_engine/stream/WavFileStream.h"

int main() {
    using clock = std::chrono::steady_clock;

    audio_engine::network::UdpSender sender("127.0.0.1", 5000);
    audio_engine::stream::WavFileStream wav(
        "/Users/akarsh_rawat/dev_projects/audio-streaming-engine/Mozart_from_Piano_Sonata_K310_first_movement.wav"
    );

    uint64_t pts = 0;       // media clock in samples
    uint32_t sequence = 0;

    const auto stream_start_time = clock::now();

    while (auto frame = wav.next_frame()) {

        // ---- Send packet ----
        sender.send_frame(*frame, sequence, pts);

        // ---- Compute when NEXT packet should be sent ----
        const double seconds_from_start =
            double(pts + frame->frames) / double(frame->sample_rate);

        const auto next_send_time =
            stream_start_time +
            std::chrono::duration_cast<clock::duration>(
                std::chrono::duration<double>(seconds_from_start));

        // ---- Advance media clock ----
        pts += frame->frames;
        sequence++;

        // ---- Media-clock pacing ----
        std::this_thread::sleep_until(next_send_time);
    }
    sender.send_eos(sequence, pts);
    return 0;
}
