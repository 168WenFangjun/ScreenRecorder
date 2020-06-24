#pragma once

#include "AudioInInterface.h"

#include <QObject>
#include <QMap>

#include <alsa/asoundlib.h>
#include <alsa/mixer.h>

class AlsaInWorker : public QObject
{
    Q_OBJECT
public:
    AlsaInWorker(snd_pcm_t *capture_handle, short *buffer, unsigned long buffer_size,
                 short bytes_per_sample, short channels, char *error);
    ~AlsaInWorker() {}
    void quit(bool quit);
public slots:
    void process();
signals:
    void finished();
    void audioData(void *audio_samples_p, unsigned long num_samples, unsigned long num_bytes_per_sample, short channels);
private:
    snd_pcm_t *m_capture_handle;
    short *m_audio_samples_p;
    short m_channels;
    unsigned long m_num_samples;
    short m_num_bytes_per_sample;
    char *m_error;
    bool m_quit;
};

class QThread;
class AlsaIn : public AudioInInterface
{
public:
    AlsaIn();

    virtual ~AlsaIn();
    virtual bool Init();
    virtual bool Open(int microphone_idx, int microphone_volume);
    virtual bool Start();
    virtual void Stop();
    virtual void Close();
    virtual QList<AudioDevice> availableAudioDevices(bool bInput);


    virtual const char* GetError() const override;
    virtual unsigned long GetBitRate() const override;
    virtual unsigned long GetSampleRate() const override;
    virtual unsigned short GetBitsPerSample() const override;
    virtual unsigned short GetNumChannels() const override;
    virtual unsigned long GetBufferSize() const override;
protected:
    snd_pcm_t *m_capture_handle;
    unsigned int m_sample_rate;
    unsigned int m_bitrate;
    short m_bytes_per_sample;
    short m_channels;
    short m_buffer[1024];
    char m_error[1024];
    QThread *m_thread;
    AlsaInWorker *m_worker;
    QList<AudioDevice> m_devices;
};
