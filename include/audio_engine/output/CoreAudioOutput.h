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
                    network::Metrics& metrics)
        : buffer_(buffer),
          metrics_(metrics)
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

        audio::AudioFrame frame;
        if (!buffer_.pop(frame)) {
            metrics_.underruns.fetch_add(1,
                                         std::memory_order_relaxed);
            std::fill(out, out + frames * 2, 0.0f);
            return noErr;
        }

        std::copy(frame.samples.begin(),
                  frame.samples.end(),
                  out);

        metrics_.frames_rendered.fetch_add(1,
                                           std::memory_order_relaxed);
        return noErr;
    }

private:
    buffer::RingBuffer<audio::AudioFrame>& buffer_;
    network::Metrics& metrics_;
    AudioUnit unit_{nullptr};
};

} // namespace audio_engine::output
