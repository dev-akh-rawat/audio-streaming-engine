#pragma once

#include <fstream>
#include <vector>
#include <optional>
#include <cstdint>
#include <cstring>

#include "audio_engine/stream/AudioStream.h"
#include "audio_engine/audio/AudioFrame.h"

namespace audio_engine::stream {

class WavFileStream : public AudioStream {
public:
    explicit WavFileStream(const std::string& path,
                           size_t frames_per_block = 256)
        : frames_per_block_(frames_per_block)
    {
        file_.open(path, std::ios::binary);
        if (!file_) {
            throw std::runtime_error("Failed to open WAV file");
        }

        parse_header();
    }

    std::optional<audio_engine::audio::AudioFrame>
    next_frame() override {
        if (!file_ || eof_) {
            return std::nullopt;
        }

        audio_engine::audio::AudioFrame frame(
            sample_rate_,
            channels_,
            frames_per_block_
        );

        const size_t bytes_per_sample = bits_per_sample_ / 8;
        const size_t bytes_to_read =
            frames_per_block_ * channels_ * bytes_per_sample;

        std::vector<char> buffer(bytes_to_read);

        file_.read(buffer.data(), bytes_to_read);
        std::streamsize bytes_read = file_.gcount();

        if (bytes_read == 0) {
            eof_ = true;
            return std::nullopt;
        }

        const size_t samples_read =
            bytes_read / bytes_per_sample;

        for (size_t i = 0; i < samples_read; ++i) {
            if (bits_per_sample_ == 16) {
                int16_t sample =
                    *reinterpret_cast<int16_t*>(&buffer[i * 2]);
                frame.samples[i] =
                    static_cast<float>(sample) / 32768.0f;
            } else if (bits_per_sample_ == 32) {
                int32_t sample =
                    *reinterpret_cast<int32_t*>(&buffer[i * 4]);
                frame.samples[i] =
                    static_cast<float>(sample) / 2147483648.0f;
            }
        }

        return frame;
    }

private:
    void parse_header() {
        char riff[4];
        file_.read(riff, 4); // "RIFF"
        file_.ignore(4);     // file size
        file_.read(riff, 4); // "WAVE"

        while (file_) {
            char chunk_id[4];
            uint32_t chunk_size = 0;

            file_.read(chunk_id, 4);
            file_.read(reinterpret_cast<char*>(&chunk_size), 4);

            if (std::strncmp(chunk_id, "fmt ", 4) == 0) {
                uint16_t audio_format;
                file_.read(reinterpret_cast<char*>(&audio_format), 2);
                file_.read(reinterpret_cast<char*>(&channels_), 2);
                file_.read(reinterpret_cast<char*>(&sample_rate_), 4);
                file_.ignore(6); // byte rate + block align
                file_.read(reinterpret_cast<char*>(&bits_per_sample_), 2);
                file_.ignore(chunk_size - 16);
            } else if (std::strncmp(chunk_id, "data", 4) == 0) {
                data_start_ = file_.tellg();
                data_size_ = chunk_size;
                break;
            } else {
                file_.ignore(chunk_size);
            }
        }
    }

private:
    std::ifstream file_;
    bool eof_{false};

    uint32_t sample_rate_{0};
    uint16_t channels_{0};
    uint16_t bits_per_sample_{0};

    size_t frames_per_block_;
    std::streampos data_start_;
    uint32_t data_size_;
};

} // namespace audio_engine::stream
