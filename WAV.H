#ifndef WAV_H
#define WAV_H

#include <stdio.h>
#include <windows.h>
#include <Mmsystem.h>

typedef struct WAVEFILEHEADER_ {
	DWORD ChunkID;// = 0x52494646;//"RIFF";
	DWORD ChunkSize;// =?;
	DWORD Format;// = 0x57415645;//"WAVE";
	DWORD Subchunk1ID;// = 0x666d7420;//"fmt ";
	DWORD Subchunk1Size;// = 16;
    WORD        wFormatTag;         /* format type */
    WORD        nChannels;          /* number of channels (i.e. mono, stereo...) */
    DWORD       nSamplesPerSec;     /* sample rate */
    DWORD       nAvgBytesPerSec;    /* for buffer estimation */
    WORD        nBlockAlign;        /* block size of data */
    WORD        wBitsPerSample;     /* number of bits per sample of mono data */
	DWORD Subchunk2ID;// = 0x64617461;//"data"
	DWORD Subchunk2Size;// = ?;
}
WAVEFILEHEADER;

typedef struct _WAVE_FILE_HEADER //same as WAVEFILEHEADER but used by WAVEIN class
{
	char FileDescriptionHeader[4];	// The string "RIFF"
	DWORD SizeOfFile;				// The file size - 8
	char WaveDescriptionHeader[4];	// The string "WAVE"
	char DescriptionHeader[4];		// The string "fmt "
	DWORD SizeOfWaveSectionChunk;	// 0x10 = The size of the WAVE type format (2 bytes) + mono/stereo flag (2 bytes) + sample rate (4 bytes) + bytes/sec (4 bytes) + block alignment (2 bytes) + bits/sample (2 bytes). 
	WORD WaveTypeFormat;			// 0x01
	WORD Channel;					// Mono (0x01) or stereo (0x02)
	DWORD SampleRate;				// Sample rate.
	DWORD BytesPerSecond;			// Bytes/Second
	WORD BlockAlignment;			// Block alignment
	WORD BitsPerSample;				// Bits/Sample
	char DataDescriptionHeader[4];	// The string "data"
	DWORD SizeOfData;				// Size of data that follows	
}
WAVE_FILE_HEADER;

typedef struct WAVERECORD_
{
	WAVEFILEHEADER wfh;
	HWAVEIN  hwi;
	WAVEFORMATEX wfx;
	WAVEHDR wh[2];
	char num_wh_buffers_qued;
	BOOL stop;
	FILE* fp;
	DWORD fpsize;
	int mp3encoder;
	FILE* fp_mp3;
}
WAVERECORD;

typedef struct WAVEPLAY_
{
	//WAVEFILEHEADER wfh;
	HWAVEOUT  hwo;
	//WAVEFORMATEX wfx;
	//WAVEHDR wh[2];
	char num_wh_buffers_qued;
	int mp3decoder;
	//FILE* fp;
}
WAVEPLAY;

class WAV; 

typedef struct PLAY_DATA_
{
	WAV* wav;
	WAVEHDR* pwh;
//	PLAYDATAPROC proc;
	int n;
}
PLAY_DATA;

#define FILE_READ_CHUNK_SIZE 10240
#define NUM_WAVE_HEADERS 10


/*
Lets you play multiple wave files simultaneously
WAV wav;
wav.PlayInit(hwnd, filename, WM_WAV_PLAY, WM_WAV_PLAYENDED, brepeat, read wav file repeatedly to play data, or load entire wav file into mem and play from mem);//setup stuff
wav.PlayStart();//start playing
wav.PlayStop();//to stop it without completing play
In your WndProc, add these events, assuming you declared them somewhere in your code:
		case WM_WAV_PLAY:
			{
				PLAY_DATA * play_data = (PLAY_DATA*) lParam;
				play_data->wav->PlayData(play_data);
			}
			break;
		case WM_WAV_PLAYENDED:
			WAV* wav = (WAV*) lParam;
			delete wav;//if it was dynamic object
			break;
If you do not use windows messages, then you can create your class derived from WAV class, and override 
the two virtual functions:
void WAV::PlayMore(PLAY_DATA *play_data)
void WAV::PlayEnded()
Do not call any functions within these two functions that will take too long, or are related to wave, or malloc related!

*/
class WAV
{
	char* m_wavfiledata[NUM_WAVE_HEADERS];
        char m_wavfile[256];
	int m_filepos;
	int m_stop;
	int m_num_wh_in_que;
	HWAVEOUT m_hwo;
	int m_event_playmore;
	int m_event_playended;
	HWND m_hWnd;
	bool m_repeat;
	char *m_wav_data;
	unsigned int m_wav_data_size;
	PLAY_DATA *m_play_data_arr;

	void PlayEnd(WAVEHDR *pwh);
public:
	WAVEFILEHEADER m_wfh;
	WAVEFORMATEX m_wfx;

        int PlayInit(int ihWnd, const char* wavfile, int event_playmore, int event_playended, bool repeat, bool play_from_file);
	void PlayStart();
	void PlayStop() { m_stop = 1; }

	void PlayData(void * in_data);
	virtual void PlayMore(PLAY_DATA *play_data);
	virtual void PlayEnded();
	char * GetWavfiledataPointer(int i) { return m_wavfiledata[i]; }
	void DecrementNumWavHdrInQ() { m_num_wh_in_que--; }
        unsigned int ReadFile(const char* wavfile, unsigned int start_pos, unsigned int num_samples_to_read, int* out_samples);
        unsigned int WriteFile(const char* wavfile, unsigned int num_samples_to_write, int* in_samples, unsigned int write_bits_per_sample, WAVEFILEHEADER *wfh);
        unsigned int CreateSineWav(char* wavfile, unsigned char max_amplitude, unsigned int hertz, unsigned int seconds,
		unsigned int max_num_waves, unsigned int *num_samples_in_filter, unsigned int *out_bytes_per_sample);
	PLAY_DATA *GetPlayData(int i) { return &m_play_data_arr[i]; }
        const char* GetFileName() const { return m_wavfile; }
        void SetFileName(const char* filename) { strcpy(m_wavfile, filename); }
	WAV();
	virtual ~WAV();

	WAVERECORD m_rec;
};

typedef void (*PLAYDATAPROC)(void *data);

#endif/*WAV_H*/
