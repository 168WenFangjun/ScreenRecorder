#include "AudioFrameProducer.h"

#include <QDebug>
#include <QMutexLocker>

namespace {
    FILE* rawfp;
}//namespace
AudioFrameProducer::AudioFrameProducer(MyEncoder& encoder, QMutex &encoder_m,
                                       QObject * parent)
    : QObject(parent)
    , m_encoder(encoder)
    , m_encoder_m(encoder_m)
    , m_continue_recording(false)
{
#if 0//defined(Win32) || defined(Win64)
    rawfp = fopen("C:\\Users\\oosman\\AppData\\Local\\ScreenRecorder\\audio.raw", "wb");
#elif defined Linux
//    rawfp = fopen("/home/oosman/work/ScreenRecorder-build/audio.raw", "wb");
//    rawfp = fopen("/home/oosman/backup/work/build-ScreenRecorder-Desktop_Qt_5_9_2_GCC_64bit-Debug/audio.raw", "wb");
#endif
}

AudioFrameProducer::~AudioFrameProducer()
{
    if(rawfp)
    {
        fclose(rawfp);
        rawfp = nullptr;
    }
}

void AudioFrameProducer::startAudioRecording()
{
    m_continue_recording = true;
}

void AudioFrameProducer::stopAudioRecording()
{
    qDebug() << "stopAudioRecording";
    m_continue_recording = false;
}

void AudioFrameProducer::enqueAudioSamples(void *audio_samples_p,
                                           unsigned long num_samples,
                                           unsigned long num_bytes_per_sample,
                                           short channels,
                                           unsigned long samples_per_frame
                                           )
{
    if(!m_continue_recording)
    {
        free(audio_samples_p);
        emit stoppedAudioRecording();
        return;
    }
    if(rawfp)
    {
        fwrite(audio_samples_p, num_samples, num_bytes_per_sample * channels, rawfp);
    }

    AVStream *st = m_encoder.audio_st;

    int n = 0;
    uint8_t* data_p = (uint8_t*)audio_samples_p;
    while(num_samples > 0)
    {
        QMutexLocker lock(&m_encoder_m);
        unsigned long num_samples_to_encode = (num_samples > samples_per_frame) ?
                    samples_per_frame: num_samples;
//        (10 frames/sec) / (44100 samples/sec) = frames/sample
//        audio_pts = m_encoder.audio_pts * frames/sample
        int64_t audio_pts = av_rescale_q(m_encoder.audio_pts, st->codec->time_base, st->time_base);
        AVFrame* frame = ::enqueAudioSamples(st,
                                             audio_pts,
                                             &data_p[n * num_bytes_per_sample * channels],
                                             num_samples_to_encode);
        m_encoder.audio_pts += frame->nb_samples;
        n += num_samples_to_encode;
        num_samples -= num_samples_to_encode;

        ::writeAudio(&m_encoder, frame);
        av_frame_free(&frame);
    }
    free(audio_samples_p);
}

