#pragma once

#include <QObject>
#include <QMutex>

#include "ffwrite.h"
#include "../DS/circularbuffer.h"

class AudioFrameProducer : public QObject
{
    Q_OBJECT
public:
    AudioFrameProducer(MyEncoder& encoder,
                       QMutex& encoder_m,
                       QObject * parent = nullptr);
    virtual ~AudioFrameProducer();

public slots:
    void startAudioRecording();
    void stopAudioRecording();
    void enqueAudioSamples(void *audio_samples_p,
                           unsigned long num_samples,
                           unsigned long num_bytes_per_sample,
                           short channels,
                           unsigned long samples_per_frame);
signals:
    void writeAudioFrame();
    void writeVideoFrame();
    void stoppedAudioRecording();

protected:
    MyEncoder &m_encoder;
    bool m_continue_recording;
    QMutex &m_encoder_m;
};

