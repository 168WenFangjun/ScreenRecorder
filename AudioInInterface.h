#pragma once

#include <QObject>
#include <QImage>

struct AudioDevice
{
    QString m_id;
    QString m_name;
    short m_channels;
    QImage m_img;

    AudioDevice(const QString &id, const QString &name, short channels, const QImage &img)
        : m_id(id)
        , m_name(name)
        , m_channels(channels)
        , m_img(img)
    {}
};

class AudioInInterface : public QObject  {

    Q_OBJECT
public:

    AudioInInterface();
    virtual ~AudioInInterface();
    virtual QList<AudioDevice> availableAudioDevices(bool bInput) = 0;
    virtual bool Init() = 0;
    virtual bool Open(int microphone_idx, int microphone_volume) = 0;
    virtual bool Start() = 0;
    virtual void Stop() = 0;
    virtual void Close() = 0;

    virtual const char* GetError() const = 0;
    virtual unsigned long GetBitRate() const = 0;
    virtual unsigned long GetSampleRate() const = 0;
    virtual unsigned short GetBitsPerSample() const = 0;
    virtual unsigned short GetNumChannels() const = 0;
    virtual unsigned long GetBufferSize() const = 0;

signals:
    void audioData(void *audio_samples_p, unsigned long num_samples, unsigned long num_bytes_per_sample, short channels);
};
