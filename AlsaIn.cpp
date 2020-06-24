#include "AlsaIn.h"

#include <QDebug>
#include <QThread>

extern "C" void mylog(const char *fmt, ...);

namespace
{
bool openAudioDevice(const QString &device, snd_pcm_t **capture_handle, char *error)
{
    int err;
    if ((err = snd_pcm_open (capture_handle,
                             device.toUtf8().data(), SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        sprintf (error, "cannot open audio device %s (%s)\n",
             device.toUtf8().data(),
             snd_strerror (err));
        return false;
    }
    return true;
}
void setAudioLevels(const char *card, int percent)
{
    snd_mixer_t *handle;
    snd_mixer_open(&handle, 0);
    snd_mixer_attach(handle, card);
    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);

    snd_mixer_selem_id_t *sid;
    if(0)
    {
        const char *selem_name = "Master";

        snd_mixer_selem_id_alloca(&sid);
        snd_mixer_selem_id_set_index(sid, 0);
        snd_mixer_selem_id_set_name(sid, selem_name);
        snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

        if (snd_mixer_selem_has_playback_switch(elem))
        {
            snd_mixer_selem_set_playback_switch_all(elem, 0);
        }
    }
    if(1)
    {
        const char *selem_name = "Capture";

        snd_mixer_selem_id_alloca(&sid);
        snd_mixer_selem_id_set_index(sid, 0);
        snd_mixer_selem_id_set_name(sid, selem_name);
        snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

        if (elem && snd_mixer_selem_has_capture_switch(elem))
        {
            long value = 0;
            snd_mixer_selem_channel_id_t  channel = SND_MIXER_SCHN_MONO;

            snd_mixer_selem_get_capture_dB(elem, channel, &value);
            qDebug() << "capture dB" << value;

            snd_mixer_selem_get_capture_volume(elem, channel, &value);
            qDebug() << "capture volume" << value;
            long min = 0;
            long max = 0;
            snd_mixer_selem_get_capture_volume_range(elem, &min, &max);
            qDebug() << "capture volume range" << min << max;

            value = percent * (max - min) / 100;
            snd_mixer_selem_set_capture_volume(elem, SND_MIXER_SCHN_MONO, value);
            snd_mixer_selem_get_capture_volume(elem, channel, &value);
            qDebug() << "capture volume" << value << (value * 100 / (max - min)) << "%";
            snd_mixer_selem_get_capture_dB(elem, channel, &value);
            qDebug() << "capture dB" << value;
        }
    }
    snd_mixer_close(handle);
}

}//namespace

AlsaInWorker::AlsaInWorker(snd_pcm_t *capture_handle, short *buffer, unsigned long buffer_size,
                           short bytes_per_sample, short channels, char *error)
    : QObject(nullptr)
    , m_capture_handle(capture_handle)
    , m_audio_samples_p(buffer)
    , m_channels(channels)
    , m_num_samples(buffer_size)
    , m_num_bytes_per_sample(bytes_per_sample)
    , m_error(error)
    , m_quit(false)
{
}

void AlsaInWorker::quit(bool quit)
{
    m_quit = quit;
}

void AlsaInWorker::process()
{
    int err;
    if ((err = snd_pcm_readi (m_capture_handle, m_audio_samples_p, m_num_samples)) != m_num_samples) {
        sprintf (m_error, "read from audio interface failed (%s)\n",
                 snd_strerror (err));
    }
    else
    {
        short *audio_samples_p = (short *)malloc(m_num_samples * m_num_bytes_per_sample * 1);
        if(m_channels == 1)
        {
            memcpy(audio_samples_p, m_audio_samples_p, m_num_samples * m_num_bytes_per_sample * 1);
        }
//        Todo: can't handle multip channels!!!
//        else
//        {
//            for(int i = 0; i < m_num_samples; ++i)
//            {
//                audio_samples_p[i] = (m_audio_samples_p[2*i] + m_audio_samples_p[2*i+1])/2;
//            }
//        }
        emit audioData(audio_samples_p, m_num_samples, m_num_bytes_per_sample, 1);
    }
    if(!m_quit)
    {
        QMetaObject::invokeMethod(this, "process",
                                  Qt::QueuedConnection);
    }
}

AlsaIn::AlsaIn()
    : AudioInInterface()
    , m_capture_handle(nullptr)
    , m_sample_rate(44100)
    , m_bitrate(128000)
    , m_bytes_per_sample(2)
    , m_channels(1)
    , m_thread(nullptr)
{
    m_error[0] = 0;
    for(int i = 0; i < 10; ++i)
    {
        availableAudioDevices(true);
    }
}

AlsaIn::~AlsaIn()
{
    Stop();
    Close();
}

QList<AudioDevice> AlsaIn::availableAudioDevices(bool bInput)
{
    QList<AudioDevice> devices;
    devices.prepend(AudioDevice("default", "Default", 1, QImage()));
//    return devices;//TODO: see problem in comment below

    snd_ctl_card_info_t *info;
    snd_pcm_info_t *pcminfo;
    snd_ctl_card_info_alloca(&info);
    snd_pcm_info_alloca(&pcminfo);

    int card = -1;
    while (snd_card_next(&card) >= 0 && card >= 0) {
        int err = 0;
        snd_ctl_t *handle;
        char name[20];
        snprintf(name, sizeof(name), "hw:%d", card);
        if ((err = snd_ctl_open(&handle, name, 0)) < 0) {
            continue;
        }

        if ((err = snd_ctl_card_info(handle, info)) < 0) {
            snd_ctl_close(handle);
            continue;
        }

        int dev = -1;
        while (snd_ctl_pcm_next_device(handle, &dev) >= 0 && dev >= 0) {
            snd_pcm_info_set_device(pcminfo, dev);
            snd_pcm_info_set_subdevice(pcminfo, 0);
            snd_pcm_info_set_stream(pcminfo, bInput ? SND_PCM_STREAM_CAPTURE
                    : SND_PCM_STREAM_PLAYBACK);
            if ((err = snd_ctl_pcm_info(handle, pcminfo)) < 0) {
                continue;
            }
            if(SND_PCM_STREAM_CAPTURE != snd_pcm_info_get_stream(pcminfo)) {
                continue;
            }

            //Problem: multiple devices qualify as audio capture devices when only one should be available
            char szDeviceID[20];
            snprintf(szDeviceID, sizeof(szDeviceID), "plughw:%d,%d", card, dev);
            const char *deviceName = snd_ctl_card_info_get_name(info);
            devices.append(AudioDevice(szDeviceID,
                                       deviceName,
                                       m_channels,
                                       QImage()));
        }

        snd_ctl_close(handle);
    }
    return devices;
}


bool AlsaIn::Init()
{
    m_devices = availableAudioDevices(true);
    return m_devices.count() > 0;
}


bool AlsaIn::Open(int microphone_idx, int microphone_volume)
{
    if(m_devices.count() == 0 )
    {
        return false;
    }
    int err;
    snd_pcm_hw_params_t *hw_params;
    m_capture_handle = nullptr;

    if(microphone_idx > -1 && microphone_idx < m_devices.count())
    {
        openAudioDevice(m_devices.at(microphone_idx).m_id, &m_capture_handle, m_error);
    }
    else
    {
        microphone_idx = 0;
        for(auto device : m_devices)
        {
            if(openAudioDevice(device.m_id, &m_capture_handle, m_error))
            {
                break;
            }
            ++microphone_idx;
        }
    }
    if(!m_capture_handle)
    {
        return false;
    }
    mylog("using audio device %s\n", m_devices.at(microphone_idx).m_id.toUtf8().data());
    setAudioLevels(m_devices.at(microphone_idx).m_id.toUtf8().data(), microphone_volume);

    if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
        sprintf (m_error, "cannot allocate hardware parameter structure (%s)\n",
             snd_strerror (err));
        return false;
    }

    if ((err = snd_pcm_hw_params_any (m_capture_handle, hw_params)) < 0) {
        sprintf (m_error, "cannot initialize hardware parameter structure (%s)\n",
             snd_strerror (err));
        return false;
    }

    if ((err = snd_pcm_hw_params_set_access (m_capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        sprintf (m_error, "cannot set access type (%s)\n",
             snd_strerror (err));
        exit (1);
    }

    if ((err = snd_pcm_hw_params_set_format (m_capture_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
        sprintf (m_error, "cannot set sample format (%s)\n",
             snd_strerror (err));
        return false;
    }

    if ((err = snd_pcm_hw_params_set_rate_near (m_capture_handle, hw_params, &m_sample_rate, 0)) < 0) {
        sprintf (m_error, "cannot set sample rate (%s)\n",
             snd_strerror (err));
        return false;
    }

    if ((err = snd_pcm_hw_params_set_channels (m_capture_handle, hw_params, m_channels)) < 0) {
        sprintf (m_error, "cannot set channel count (%s)\n",
             snd_strerror (err));
        return false;
    }

    if ((err = snd_pcm_hw_params (m_capture_handle, hw_params)) < 0) {
        sprintf (m_error, "cannot set parameters (%s)\n",
             snd_strerror (err));
        return false;
    }

    snd_pcm_hw_params_free(hw_params);


    if ((err = snd_pcm_prepare (m_capture_handle)) < 0) {
                sprintf (m_error, "cannot prepare audio interface for use (%s)\n",
                     snd_strerror (err));
                exit (1);
            }

    m_worker = new AlsaInWorker(m_capture_handle,
                                m_buffer,
                                sizeof(m_buffer)/sizeof(short)/m_channels,
                                m_bytes_per_sample,
                                m_channels,
                                m_error);

    connect(m_worker, &AlsaInWorker::audioData,
            this, &AlsaIn::audioData, Qt::QueuedConnection);

    return true;
}

bool AlsaIn::Start()
{
    if(!m_capture_handle || !m_worker)
    {
        return false;
    }
    m_thread = new QThread;
    m_worker->moveToThread(m_thread);
    QObject::connect(m_thread, &QThread::started, m_worker, &AlsaInWorker::process);
    QObject::connect(m_worker, &AlsaInWorker::finished, m_thread, &QThread::quit);
    QObject::connect(m_worker, &AlsaInWorker::finished, m_worker, &AlsaInWorker::deleteLater);
    QObject::connect(m_thread, &QThread::finished, m_thread, &QThread::deleteLater);
    m_worker->quit(false);
    m_thread->start();

    return true;
}

void AlsaIn::Stop()
{
    if(m_thread)
    {
        if(m_worker)
        {
            m_worker->quit(true);
        }
        m_thread->quit();
        m_thread->wait();
        m_thread = nullptr;
    }
    if(m_worker)
    {
        m_worker = nullptr;
    }
}

void AlsaIn::Close()
{
    if(m_capture_handle)
    {
        snd_pcm_close (m_capture_handle);
        m_capture_handle = nullptr;
    }
}

const char* AlsaIn::GetError() const
{
    return m_error;
}

unsigned long AlsaIn::GetBitRate() const
{
    return m_bitrate;
}

unsigned long AlsaIn::GetSampleRate() const
{
    return m_sample_rate;
}

unsigned short AlsaIn::GetBitsPerSample() const
{
    return m_bytes_per_sample * 8;
}

unsigned short AlsaIn::GetNumChannels() const
{
    return m_channels;//TODO: does not work for 2 channels
}

unsigned long AlsaIn::GetBufferSize() const
{
    return sizeof(m_buffer);
}

