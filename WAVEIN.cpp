// WAVIN.cpp : Defines the entry point for the DLL application.
//

#include "../fileio/fileio.h"
#include "WAVEIN.h"
#include <stdio.h>

#include <QDebug>

// This is the constructor of a class that has been exported.
// see WAV.h for the class definition
WAVEIN::WAVEIN()
    :m_frame_size(2048)
{ 
    int i;
    m_error[0] = 0;

    for(i=0; i<MAX_BUFFERS; i++)
    {
        m_wav_buffer[i]=0;
    }

    // WAVE STUFF INITIALIZATION
    m_hwi = 0;
    m_sample_rate = 44100;//8000|11025|22050|44100|96000
    m_bits_per_sample = 16;//8|16
    m_channels = 1;
    //16000 bytes per second
    //44100 samples per second
    //16 bytes per sample
    m_bitrate = 128000;//m_sample_rate * m_channels * m_bits_per_sample;
    m_block_alignment = m_channels * m_bits_per_sample / 8;

    m_next_buffer_to_enque = 0;
    OnWaveCaptured = nullptr;

    return;
}

WAVEIN::~WAVEIN()
{
    Close();
    FreeWaveBuffers();
}

bool WAVEIN::Init()
{
    WriteInit();

    if(!AllocateWaveBuffers())
    {
        return false;
    }

    return true;
}

unsigned int WAVEIN::GetAudioInputDeviceId()
{	
    WAVEINCAPS wic = {0};
    int numdevices = waveInGetNumDevs();
    MMRESULT res = 0;
    unsigned int id = -1;

    for(int i=0; i<numdevices; i++)
    {
        res = waveInGetDevCaps(i, &wic,  sizeof(wic));
        if(wic.dwFormats & 0xfff)//see formats in c:\visualstudio8\vc\platformsdk\include\mmsystem.h, starting from WAVE_INVALIDFORMAT
        {
            id = i;
            break;//found our audio input device
        }
    }

    if(id==-1)
    {
        char err[512]={0};
        waveInGetErrorText(res, err, sizeof(err));
        sprintf(m_error, "waveInGetDevCaps() error: %s", err);
    }

    return id;
}

QList<AudioDevice> WAVEIN::availableOutputDevices()
{
    QList<AudioDevice> devices;

    WAVEOUTCAPS woc = {0};
    int numdevices = waveOutGetNumDevs();
    MMRESULT res = 0;

    for(int i = 0; i < numdevices; i++)
    {
        res = waveOutGetDevCaps(i, &woc,  sizeof(woc));
        if(woc.dwFormats & 0xfff)
        {
            devices.append(AudioDevice(QString("Audio Output %1").arg(i + 1),
                                       QString(woc.szPname),
                                       (short)woc.wChannels,
                                       QImage()));
        }
    }

    if(devices.count() == 0)
    {
        char err[1024]={0};
        waveOutGetErrorText(res, err, sizeof(err));
        sprintf(m_error, "waveOutGetDevCaps() error: %s", err);
    }

    return devices;
}

QList<AudioDevice> WAVEIN::availableInputDevices()
{
    QList<AudioDevice> devices;

    WAVEINCAPS wic = {0};
    int numdevices = waveInGetNumDevs();
    MMRESULT res = 0;

    for(int i = 0; i < numdevices; i++)
    {
        res = waveInGetDevCaps(i, &wic,  sizeof(wic));
        if(wic.dwFormats & 0xfff)//see formats https://msdn.microsoft.com/en-us/library/windows/desktop/dd743839(v=vs.85).aspx
        {
            devices.append(AudioDevice(QString("Audio Input %1").arg(i + 1),
                                       QString(wic.szPname),
                                       (short)wic.wChannels,
                                       QImage()));
        }
    }

    if(devices.count() == 0)
    {
        char err[1024]={0};
        waveInGetErrorText(res, err, sizeof(err));
        sprintf(m_error, "waveInGetDevCaps() error: %s", err);
    }

    return devices;
}

QList<AudioDevice> WAVEIN::availableAudioDevices(bool bInput)
{
    QList<AudioDevice> devices;
    if(!bInput)
    {
        return devices;//todo, get output devices
    }

    devices = availableInputDevices();
//    devices.append(availableOutputDevices());

    return devices;
}

bool WAVEIN::Open(int microphone_idx, int microphone_volume)
{
    bool bret = FALSE;
    MMRESULT res = 0;

    WAVEFORMATEX waveformatex =
    {
        WAVE_FORMAT_PCM,
        m_channels,
        m_sample_rate,
        m_bitrate/8,
        m_block_alignment,
        m_bits_per_sample,
        sizeof(WAVEFORMATEX)
    };

//    auto devices = availableAudioDevices(true);
//    if(microphone_idx < 0 || microphone_idx >= devices.count())
//    {
//        goto Exit;
//    }

    res = waveInOpen(&m_hwi, microphone_idx, &waveformatex, (DWORD_PTR)waveInProc, (DWORD_PTR)this, CALLBACK_FUNCTION);
    if(res!=MMSYSERR_NOERROR)
    {
        char err[512]={0};
        waveInGetErrorText(res, err, sizeof(err));
        sprintf(m_error, "waveInOpen() error: %s", err);
        goto Exit;
    }

    if(!PrepareWaveInHeaders())
        goto Exit;

    bret = TRUE;
Exit:

    return bret;
}

void WAVEIN::Close()
{
    if(m_hwi)
    {
        waveInReset(m_hwi);
        UnprepareWaveInHeaders();
        m_hwi = 0;
    }
}

bool WAVEIN::PrepareWaveInHeaders()
{
    bool ret = 0;
    MMRESULT res = 0;
    int i;

    for(i=0; i<MAX_BUFFERS; i++)
    {
        m_wav_header[i].lpData = m_wav_buffer[i];
        m_wav_header[i].dwBufferLength = m_frame_size;
        m_wav_header[i].dwFlags = 0;

        res = waveInPrepareHeader(m_hwi, &m_wav_header[i], sizeof(WAVEHDR));
        if(MMSYSERR_NOERROR != res)
        {
            char err[512]={0};
            waveInGetErrorText(res, err, sizeof(err));
            sprintf(m_error, "waveInPrepareHeader() error: %s", err);
            goto Exit;
        }
    }

    ret = 1;
Exit:
    return ret;
}

bool WAVEIN::UnprepareWaveInHeaders()
{
    bool ret = 0;
    MMRESULT res = 0;
    int i;

    for(i=0; i<MAX_BUFFERS; i++)
    {
        waveInUnprepareHeader(m_hwi, &m_wav_header[i], sizeof(WAVEHDR));
        if(MMSYSERR_NOERROR != res)
        {
            char err[512]={0};
            waveInGetErrorText(res, err, sizeof(err));
            sprintf(m_error, "waveInUnprepareHeader() error: %s", err);
            goto Exit;
        }
    }

    ret = 1;
Exit:
    return ret;
}

int WAVEIN::AddBuffersToWaveInQue(int nBuffersToEnque)
{
    bool bret = TRUE;
    MMRESULT res = 0;
    int j=0;

    if(nBuffersToEnque>MAX_BUFFERS)
    {
        sprintf(m_error, "Cannot exceed number of max buffers that can be enqued: %i", MAX_BUFFERS);
        goto Exit;
    }

    for(j=0; j<nBuffersToEnque; j++)
    {
        res = waveInAddBuffer(m_hwi, &m_wav_header[m_next_buffer_to_enque], sizeof(WAVEHDR));
        //if(WAVERR_STILLPLAYING == res)
        //{
        //	break;
        //}
        //else
        if(MMSYSERR_NOERROR != res)
        {
            char err[512]={0};
            waveInGetErrorText(res, err, sizeof(err));
            sprintf(m_error, "waveInAddBuffer() error: %s", err);
            goto Exit;
        }

        m_next_buffer_to_enque = (m_next_buffer_to_enque+1)%MAX_BUFFERS;
    }

Exit:
    return j;//numbers of buffers successfully enqued
}

bool WAVEIN::Start()
{
    bool ret = 0;
    MMRESULT res = 0;
    int BuffersEnqued;

    if(!m_hwi)
    {
        sprintf(m_error, "%s", "Start() error: wave handle not initialized");
        goto Exit;
    }


    BuffersEnqued = AddBuffersToWaveInQue(MAX_BUFFERS);
    if(!BuffersEnqued)
    {
        goto Exit;
    }

    res = waveInStart(m_hwi);
    if(MMSYSERR_NOERROR != res)
    {
        char err[512]={0};
        waveInGetErrorText(res, err, sizeof(err));
        sprintf(m_error, "waveInStart() error: %s", err);
        goto Exit;
    }

    ret = 1;
Exit:
    return ret;

}

void WAVEIN::Stop()
{
    if(m_hwi)
    {
        waveInReset(m_hwi);
    }
}

void WAVEIN::waveInProc(HWAVEIN hwi, unsigned int uMsg, void* dwInstance, void* dwParam1, void* dwParam2)
{
    WAVEIN * pThis = (WAVEIN*) dwInstance;
    switch(uMsg)
    {
    case WIM_DATA:
    {
        WAVEHDR * pwvhdr = (WAVEHDR *)dwParam1;
        unsigned short bytes_per_sample = pThis->m_bits_per_sample/8;

        if(pThis->receivers(SIGNAL(audioData(void *, unsigned long, unsigned long, short))))
        {
            void *audio_samples_p = malloc(pwvhdr->dwBytesRecorded);
            memcpy(audio_samples_p, pwvhdr->lpData, pwvhdr->dwBytesRecorded);
            emit pThis->audioData(audio_samples_p,
                                  pwvhdr->dwBytesRecorded/bytes_per_sample,
                                  bytes_per_sample,
                                  1);
        }
        pThis->AddBuffersToWaveInQue(pwvhdr->dwBytesRecorded/pThis->m_frame_size);
    }
        break;
    case WIM_OPEN:
        break;
    case WIM_CLOSE:
        qDebug() << "wim_close";
        break;
    }
}

//This funciton should be called the first time with SizeOfData=0
//so the header is initialized. Later when data is passed, it is
//appended in the data portion after the header, and the header
//is updated with the correct size of data.
//
void WAVEIN::WriteInit()
{
    m_wav_file_size = sizeof(WAVE_FILE_HEADER);

    memcpy(&m_wf.FileDescriptionHeader, "RIFF", 4);
    memcpy(&m_wf.WaveDescriptionHeader, "WAVE", 4);
    memcpy(&m_wf.DescriptionHeader, "fmt ", 4);
    m_wf.SizeOfWaveSectionChunk = 0x10;
    m_wf.WaveTypeFormat = 0x01;
    m_wf.Channel = m_channels;
    m_wf.SampleRate = m_sample_rate;
    m_wf.BytesPerSecond = m_bitrate/8;
    m_wf.BlockAlignment = m_block_alignment;
    m_wf.BitsPerSample = m_bits_per_sample;
    memcpy(&m_wf.DataDescriptionHeader, "data", 4);
    m_wf.SizeOfData = 0;
    m_wf.SizeOfFile = m_wav_file_size - 8;
}

bool WAVEIN::WriteStart()
{
    bool bret = FALSE;
    FILE*fp = fopen(m_filename, "wb");
    if(fp)
    {
        fwrite(&m_wf, sizeof(WAVE_FILE_HEADER), 1, fp);
        fclose(fp);
        bret = TRUE;
    }

    return bret;
}

bool WAVEIN::WriteData(char *pData, unsigned long DataSize)
{
    bool bret = FALSE;

    FILE*fp = fopen(m_filename, "a+b");

    if(fp)
    {
        if(fwrite(pData, DataSize, 1, fp))
        {
            m_wav_file_size += DataSize;
            m_wf.SizeOfData += DataSize;
            m_wf.SizeOfFile += DataSize;
        }
        fclose(fp);
        bret = TRUE;
    }

    return bret;
}

bool WAVEIN::WriteEnd()
{
    bool bret = FALSE;
    fpos_t pos;
    FILE*fp = fopen(m_filename, "r+b");

    if(fp)
    {
        pos = 4;
        fsetpos(fp, &pos);
        fwrite(&m_wf.SizeOfFile, 4, 1, fp);
        pos = sizeof(WAVE_FILE_HEADER)-4;
        fsetpos(fp, &pos);
        fwrite(&m_wf.SizeOfData, 4, 1, fp);
        fclose(fp);
        bret = TRUE;
    }

    return bret;
}

unsigned int WAVEIN::AllocateWaveBuffers()
{
    unsigned int ret = 0;
    int i;
    for(i=0; i<MAX_BUFFERS; i++)
    {
        m_wav_buffer[i] = (char*) realloc(m_wav_buffer[i], m_frame_size);
        if(!m_wav_buffer[i])
            goto Exit;
    }

    ret = 1;
Exit:
    if(ret==0)
        sprintf(m_error, "%s", "Could not allocate enough memory for wave buffers");
    return ret;
}

void WAVEIN::FreeWaveBuffers()
{
    int i;
    for(i=0; i<MAX_BUFFERS; i++)
    {
        if(m_wav_buffer[i])
        {
            free(m_wav_buffer[i]);
            m_wav_buffer[i]=0;
        }
    }
}

const char* WAVEIN::GetError() const  { return m_error; }
unsigned long WAVEIN::GetBitRate() const  { return m_bitrate; }
unsigned long WAVEIN::GetSampleRate() const  { return m_sample_rate; }
unsigned short WAVEIN::GetBitsPerSample() const  { return m_bits_per_sample; }
unsigned short WAVEIN::GetNumChannels() const  { return m_channels; }
unsigned long WAVEIN::GetBufferSize() const  { return m_frame_size; }
