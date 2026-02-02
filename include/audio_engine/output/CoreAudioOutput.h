#pragma once

#include <AudioToolbox/AudioToolbox.h>
#include <atomic>

#include "audio_engine/buffer/RingBuffer.h"
#include "audio_engine/audio/AudioFrame.h"

namespace audio_engine::output {

class CoreAudioOutput {
public:
    explicit CoreAudioOutput(buffer::RingBuffer<audio::AudioFrame>& buffer)
        : buffer_(buffer)
    {}

    bool start() {
        AudioComponentDescription desc{};
        desc.componentType = kAudioUnitType_Output;
        desc.componentSubType = kAudioUnitSubType_DefaultOutput;
        desc.componentManufacturer = kAudioUnitManufacturer_Apple;

        AudioComponent component = AudioComponentFindNext(nullptr, &desc);
        if (!component) return false;

        if (AudioComponentInstanceNew(component, &unit_) != noErr)
            return false;

        AudioStreamBasicDescription format{};
        format.mSampleRate       = 48000;
        format.mFormatID         = kAudioFormatLinearPCM;
        format.mFormatFlags      = kAudioFormatFlagIsFloat |
                                   kAudioFormatFlagIsPacked;
        format.mBitsPerChannel   = 32;
        format.mChannelsPerFrame = 2;
        format.mFramesPerPacket  = 1;
        format.mBytesPerFrame    = 8;
        format.mBytesPerPacket   = 8;

        AudioUnitSetProperty(
            unit_,
            kAudioUnitProperty_StreamFormat,
            kAudioUnitScope_Input,
            0,
            &format,
            sizeof(format)
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
        running_.store(true);
        return true;
    }

    void stop() {
        running_.store(false);
        AudioOutputUnitStop(unit_);
        AudioUnitUninitialize(unit_);
        AudioComponentInstanceDispose(unit_);
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
        float* out = static_cast<float*>(ioData->mBuffers[0].mData);

        audio::AudioFrame frame;
        if (!buffer_.pop(frame)) {
            // Output silence if no data
            std::fill(out, out + frames * 2, 0.0f);
            return noErr;
        }

        std::copy(
            frame.samples.begin(),
            frame.samples.end(),
            out
        );

        return noErr;
    }

private:
    buffer::RingBuffer<audio::AudioFrame>& buffer_;
    AudioUnit unit_{nullptr};
    std::atomic<bool> running_{false};
};

} // namespace audio_engine::output
