#pragma once

#include "AudioInInterface.h"

class NullAudioIn : public AudioInInterface
{
public:
    NullAudioIn() : AudioInInterface() {}
    virtual bool Init() override { return true; }
    virtual bool Open(int microphone_idx, int microphone_volume) override { return true; }
    virtual bool Start() override { return true; }
    virtual void Stop() override {}
    virtual void Close() override { }

    const char* GetError() const override { return "no error"; }
    virtual unsigned long GetBitRate() const override { return 0; }
    virtual unsigned long GetSampleRate() const override { return 0; }
    virtual unsigned short GetBitsPerSample() const override { return 0; }
    virtual unsigned short GetNumChannels() const override { return 0; }
    virtual unsigned long GetBufferSize() const override { return 0; }
};

