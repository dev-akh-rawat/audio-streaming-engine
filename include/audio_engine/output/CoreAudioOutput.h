#pragma once

#include <AudioToolbox/AudioToolbox.h>
#include <atomic>
#include <algorithm>

#include "audio_engine/buffer/RingBuffer.h"
#include "audio_engine/audio/AudioFrame.h"
#include "audio_engine/network/Metrics.h"

namespace audio_engine::output {

/*
 * CoreAudioOutput:
 * - Consumes AudioFrames from RingBuffer
 * - Runs inside Core Audio real-time callback
 *
 * NO allocation, NO blocking in callback.
 */
class CoreAudioOutput {
public:
    CoreAudioOutput(buffer::RingBuffer<audio::AudioFrame>& buffer,
                    network::Metrics& metrics,
                    std::atomic<bool>& stream_started)
        : buffer_(buffer),
          metrics_(metrics),
          stream_started_(stream_started)
    {}

    bool start() {
        AudioComponentDescription desc{};
        desc.componentType = kAudioUnitType_Output;
        desc.componentSubType = kAudioUnitSubType_DefaultOutput;
        desc.componentManufacturer = kAudioUnitManufacturer_Apple;

        AudioComponent comp = AudioComponentFindNext(nullptr, &desc);
        if (!comp) return false;

        if (AudioComponentInstanceNew(comp, &unit_) != noErr)
            return false;

        AudioStreamBasicDescription fmt{};
        fmt.mSampleRate       = 48000;
        fmt.mFormatID         = kAudioFormatLinearPCM;
        fmt.mFormatFlags      = kAudioFormatFlagIsFloat |
                                 kAudioFormatFlagIsPacked;
        fmt.mBitsPerChannel   = 32;
        fmt.mChannelsPerFrame = 2;
        fmt.mFramesPerPacket  = 1;
        fmt.mBytesPerFrame    = 8;
        fmt.mBytesPerPacket   = 8;

        AudioUnitSetProperty(
            unit_,
            kAudioUnitProperty_StreamFormat,
            kAudioUnitScope_Input,
            0,
            &fmt,
            sizeof(fmt)
        );

        AURenderCallbackStruct cb{};
        cb.inputProc = &CoreAudioOutput::render_callback;
        cb.inputProcRefCon = this;

        AudioUnitSetProperty(
            unit_,
            kAudioUnitProperty_SetRenderCallback,
            kAudioUnitScope_Input,
            0,
            &cb,
            sizeof(cb)
        );

        AudioUnitInitialize(unit_);
        AudioOutputUnitStart(unit_);
        return true;
    }

    void stop() {
        AudioOutputUnitStop(unit_);
        AudioUnitUninitialize(unit_);
        AudioComponentInstanceDispose(unit_);
        unit_ = nullptr;
    }

private:
    static OSStatus render_callback(
        void* refCon,
        AudioUnitRenderActionFlags*,
        const AudioTimeStamp*,
        UInt32,
        UInt32 frames,
        AudioBufferList* ioData
    ) {
        return static_cast<CoreAudioOutput*>(refCon)
            ->render(ioData, frames);
    }

    OSStatus render(AudioBufferList* ioData, UInt32 frames) {
        float* out =
            static_cast<float*>(ioData->mBuffers[0].mData);

        const size_t samples_requested = frames * 2; // stereo
        size_t samples_written = 0;

        // 🔑 Stream not started yet → silence, no underrun
        if (!stream_started_.load(std::memory_order_acquire)) {
            std::fill(out, out + samples_requested, 0.0f);
            return noErr;
        }

        while (samples_written < samples_requested) {

            // Need a new frame?
            if (frame_offset_ >= current_frame_.samples.size()) {
                if (!buffer_.pop(current_frame_)) {
                    metrics_.underruns.fetch_add(
                        1, std::memory_order_relaxed);
                    std::fill(out + samples_written,
                              out + samples_requested,
                              0.0f);
                    return noErr;
                }
                frame_offset_ = 0;
            }

            const size_t available =
                current_frame_.samples.size() - frame_offset_;

            const size_t to_copy =
                std::min(available,
                         samples_requested - samples_written);

            std::copy_n(
                current_frame_.samples.data() + frame_offset_,
                to_copy,
                out + samples_written
            );

            frame_offset_ += to_copy;
            samples_written += to_copy;
        }

        metrics_.frames_rendered.fetch_add(
            1, std::memory_order_relaxed);

        return noErr;
    }

private:
    buffer::RingBuffer<audio::AudioFrame>& buffer_;
    network::Metrics& metrics_;
    std::atomic<bool>& stream_started_;
    AudioUnit unit_{nullptr};

    // 🔑 State for partial frame consumption
    audio::AudioFrame current_frame_;
    size_t frame_offset_{0};
};

} // namespace audio_engine::output
