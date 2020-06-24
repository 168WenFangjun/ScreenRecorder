/*
        Prepare MAX_BUFFERS buffers, each 1024*BUFFER_SIZE bytes big. Try to enque as many
        of these as you can into the wave device driver que, extra buffers will be rejected.
        When wave data is available, the driver copies the data into its que, and your
        callback is invoked. You get the wave data in your callback
*/

#include <functional>
#include "AudioInInterface.h"
#include "WAV.H"

#define MAX_BUFFERS 40//max buffers that can be enqued into the wave in que

// This class is exported from the WAV.dll
class WAVEIN : public AudioInInterface {

    HANDLE m_wav_in_h;//wave-in handle
    WAVE_FILE_HEADER m_wf;
    unsigned long m_wav_file_size;
    unsigned long m_sample_rate;
    unsigned short m_bits_per_sample;
    unsigned long m_bitrate;
    unsigned short m_channels;
    unsigned short m_block_alignment;
    const unsigned long m_frame_size;
    char *m_wav_buffer[MAX_BUFFERS];
    char m_error[1024];
    char m_filename[32];

    WAVEHDR m_wav_header[MAX_BUFFERS];
    HWAVEIN m_hwi;
    int	m_next_buffer_to_enque;

    UINT GetAudioInputDeviceId();
    bool PrepareWaveInHeaders();
    bool UnprepareWaveInHeaders();
    int AddBuffersToWaveInQue(int nBuffersToEnque);
    static void CALLBACK waveInProc(HWAVEIN hwi, unsigned int uMsg, void *dwInstance, void *dwParam1, void *dwParam2);
    //    typedef bool(*ON_WAVE_CAPTURED)(unsigned long BytesRecorded, void *pRecordedData, void* pUserData);
    //	ON_WAVE_CAPTURED fnOnWaveCaptured;
    std::function<bool (void *audio_samples_p, unsigned long num_samples, unsigned long num_bytes_per_sample)>OnWaveCaptured;

    UINT AllocateWaveBuffers();
    void FreeWaveBuffers();

    void WriteInit();
    bool WriteStart();
    bool WriteData(char *pData, unsigned long DataSize);
    bool WriteEnd();

public:

    WAVEIN(void);
    virtual ~WAVEIN();
    //Call Init() after creating your WAVEIN object
    virtual bool Init() override;
    //Provide a callback function that will be invoked when data is recorded. Optionally pass pUserData
    //that will be a parameter when the callback is invoked. If in your implementation of the callback,
    //you return true, then recording continues, but if you return false, then recording stops.
    //!!Do not call any of the WAVEIN functions from within your callback!!!
    virtual bool Open(int microphone_idx, int microphone_volume) override;
    //Begin recording after you Init() and Open() by calling Start()
    virtual bool Start() override;
    //End recording by calling this
    virtual void Stop() override;
    //Called in destructor so no need to call, unless you are changing quality and starting over again.
    virtual void Close() override;

    virtual const char* GetError() const override;
    virtual unsigned long GetBitRate() const override;
    virtual unsigned long GetSampleRate() const override;
    virtual unsigned short GetBitsPerSample() const override;
    virtual unsigned short GetNumChannels() const override;
    virtual unsigned long GetBufferSize() const override;
    virtual QList<AudioDevice> availableAudioDevices(bool bInput) override;
protected:
    QList<AudioDevice> availableInputDevices();
    QList<AudioDevice> availableOutputDevices();
};

