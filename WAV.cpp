#include "../FILEIO/fileio.h"
#include "../ds/ds.h"
#include "WAV.h"
#include "math.h"

/*
The canonical WAVE format starts with the RIFF header:

0         4   ChunkID          Contains the letters "RIFF" in ASCII form
                               (0x52494646 big-endian form).
4         4   ChunkSize        36 + SubChunk2Size, or more precisely:
                               4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
                               This is the size of the rest of the chunk 
                               following this number.  This is the size of the 
                               entire file in bytes minus 8 bytes for the
                               two fields not included in this count:
                               ChunkID and ChunkSize.
8         4   Format           Contains the letters "WAVE"
                               (0x57415645 big-endian form).

The "WAVE" format consists of two subchunks: "fmt " and "data":
The "fmt " subchunk describes the sound data's format:

12        4   Subchunk1ID      Contains the letters "fmt "
                               (0x666d7420 big-endian form).
16        4   Subchunk1Size    16 for PCM.  This is the size of the
                               rest of the Subchunk which follows this number.
20        2   AudioFormat      PCM = 1 (i.e. Linear quantization)
                               Values other than 1 indicate some 
                               form of compression.
22        2   NumChannels      Mono = 1, Stereo = 2, etc.
24        4   SampleRate       8000, 44100, etc.
28        4   ByteRate         == SampleRate * NumChannels * BitsPerSample/8
32        2   BlockAlign       == NumChannels * BitsPerSample/8
                               The number of bytes for one sample including
                               all channels. I wonder what happens when
                               this number isn't an integer?
34        2   BitsPerSample    8 bits = 8, 16 bits = 16, etc.
          2   ExtraParamSize   if PCM, then doesn't exist
          X   ExtraParams      space for extra parameters

The "data" subchunk contains the size of the data and the actual sound:

36        4   Subchunk2ID      Contains the letters "data"
                               (0x64617461 big-endian form).
40        4   Subchunk2Size    == NumSamples * NumChannels * BitsPerSample/8
                               This is the number of bytes in the data.
                               You can also think of this as the size
                               of the read of the subchunk following this 
                               number.
44        *   Data             The actual sound data.
*/

/*------------------------------------------------------------------------------------------------------------*/

void WAV::PlayData(void * in_data)
{
#if 0
	FILEIO fp;
	DWORD num_bytes;
	PLAY_DATA* play_data = (PLAY_DATA*) in_data;
	WAVEHDR *pwh = play_data->pwh;
	WAV* pthis = play_data->wav;
	int i=play_data->n;


	if(!pthis->m_hwo)
		return;
	if(m_stop)
	{
		pthis->PlayEnd(pwh);
		pthis->m_filepos=0;
		return;
	}
    if(!pthis->m_wavfile || !*pthis->m_wavfile)
		return;

	if(!m_wav_data)
	{
        if(!fp.open(pthis->m_wavfile, FILEIO_MODE_READ|FILEIO_MODE_OPEN_EXISTING))
			return;

		fp.seek(pthis->m_filepos, FILEIO_BEGIN);
		num_bytes=fp.read(pthis->m_wavfiledata[i], FILE_READ_CHUNK_SIZE);
		if(m_repeat && num_bytes==0)
		{
			pthis->m_filepos=0;
			fp.seek(pthis->m_filepos, FILEIO_BEGIN);
			num_bytes=fp.read(pthis->m_wavfiledata[i], FILE_READ_CHUNK_SIZE);
		}
		fp.close();
	}
	else
	{
		num_bytes = ((unsigned int)(pthis->m_filepos+FILE_READ_CHUNK_SIZE) > m_wav_data_size) ? m_wav_data_size-pthis->m_filepos : FILE_READ_CHUNK_SIZE;
		if(m_repeat && num_bytes==0)
		{
			pthis->m_filepos=0;
			num_bytes = ((unsigned int)(pthis->m_filepos+FILE_READ_CHUNK_SIZE) > m_wav_data_size) ? m_wav_data_size-pthis->m_filepos : FILE_READ_CHUNK_SIZE;
		}
		memcpy(pthis->m_wavfiledata[i], m_wav_data+pthis->m_filepos, num_bytes);
	}

	pthis->m_filepos+=num_bytes;
	pwh->dwBufferLength = num_bytes;
	pwh->lpData = pthis->m_wavfiledata[i];

	if(!num_bytes)
		pthis->PlayEnd(pwh);
	else
	{
		pthis->m_num_wh_in_que++;
		::waveOutWrite(pthis->m_hwo, pwh, sizeof(WAVEHDR));
	}

	return;
#endif
}

void WAV::PlayEnd(WAVEHDR *pwh)
{
	if(pwh) 
	{
		waveOutUnprepareHeader(m_hwo, pwh, sizeof(WAVEHDR));
		free(pwh);
		pwh=0;
	}
	if(m_num_wh_in_que==0)
	{
		int i;
		for(i=0; i<NUM_WAVE_HEADERS; i++)
		{
			if(m_wavfiledata[i]) 
				free(m_wavfiledata[i]);
			m_wavfiledata[i]=0;
		}

		if(m_wav_data)
		{
			free(m_wav_data);
			m_wav_data=0;
			m_wav_data_size=0;
		}

		if(m_hwo)
		{
			waveOutClose(m_hwo);
			m_hwo=0;
		}

		PlayEnded();
	}
}

void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD* dwInstance, DWORD* dwParam1, DWORD* dwParam2)
{
	int i;
	WAV *pthis;
	WAVEHDR *pwh;
	PLAY_DATA *play_data = 0;

	switch(uMsg)
	{
	case WOM_OPEN:
		break;
	case WOM_CLOSE:
		break;
	case WOM_DONE:
		pthis = (WAV*) dwInstance;
		pwh = (WAVEHDR*)dwParam1;
		for(i=0; i<NUM_WAVE_HEADERS; i++)
		{
			if(pthis->GetWavfiledataPointer(i)==pwh->lpData)
				break;
		}
		//play_data = (PLAY_DATA*) malloc(sizeof(PLAY_DATA));
		play_data = pthis->GetPlayData(i);
		if(play_data) 
		{
			play_data->pwh = pwh;
			play_data->wav = pthis;
			//play_data->n = i;
			pthis->PlayMore(play_data);
		}
		pthis->DecrementNumWavHdrInQ();
		break;
	}
}

void WAV::PlayMore(PLAY_DATA *play_data)
{
    PostMessage(m_hWnd, m_event_playmore, 0, (LPARAM)play_data);
}

void WAV::PlayEnded()
{
    PostMessage(m_hWnd, m_event_playended, 0, (LPARAM)this);
}

WAV::WAV()
{
	int i;
	for(i=0; i<NUM_WAVE_HEADERS; i++)
	{
		m_wavfiledata[i]=0;
	}
	m_hWnd=0;
	m_num_wh_in_que=0;
	m_stop=0;
	m_hwo=0;
	m_repeat=0;
	m_wav_data=0;
	m_wav_data_size=0;
	m_play_data_arr=0;
}

WAV::~WAV()
{
	if(m_hwo)
	{
		waveOutClose(m_hwo);
		m_hwo=0;
	}
	if(m_wav_data)
		free(m_wav_data);
	if(m_play_data_arr)
		free(m_play_data_arr);
	m_wav_data=0;
	m_wav_data_size=0;
	m_hWnd=0;
	m_stop=0;
}
/*
void CALLBACK WaveStart(HWND hWnd, UINT uMsg, UINT_PTR idEvent,DWORD dwTime)
{
	WAVEHDR *pwh = (WAVEHDR *)idEvent;

	KillTimer(hWnd, idEvent);
	::waveOutWrite(m_hwo, pwh, sizeof(WAVEHDR));
}
*/
int WAV::PlayInit(int ihWnd, const char* wavfile, int event_playmore, int event_playended, bool repeat, bool play_from_file)
{
#if 0
    HWND hWnd = (HWND) ihWnd;
	MMRESULT res=0;
	UINT_PTR      uDeviceID=0;
	UINT num_audio_dev=0;
	WAVEFILEHEADER wfh;
	WAVEFORMATEX wfx;
	WAVEHDR *wh[NUM_WAVE_HEADERS];
	int bytes_read, i;
	PLAY_DATA *play_data = 0;
	FILEIO fp;
//	int mp3=1;

	m_repeat=repeat;

	memset(wh, 0, sizeof(WAVEHDR*)*NUM_WAVE_HEADERS);

	m_event_playmore = event_playmore;
	m_event_playended = event_playended;

	if(m_hwo)
	{//already playing a file, use new instance of class to play another file
		return 0;
	}
	if(!hWnd)
	{
		goto Exit1;
	}
	m_hWnd=hWnd;

	num_audio_dev = ::waveOutGetNumDevs();
	if(num_audio_dev==0)
		goto Exit1;

    strcpy(m_wavfile, wavfile);
//	if(mp3)
//		mp3_to_wave("out.mp3", "out.wav");
    if(!fp.open(m_wavfile, FILEIO_MODE_READ|FILEIO_MODE_OPEN_EXISTING))
		goto Exit1;
	bytes_read=fp.read(&wfh, sizeof(WAVEFILEHEADER));
	m_filepos=bytes_read;
	m_wav_data_size=fp.size()-bytes_read;

	if(play_from_file)
		m_wav_data=0;
	else
	{
		m_wav_data = (char*) malloc(m_wav_data_size);
		if(m_wav_data)
			fp.read(m_wav_data, m_wav_data_size);
	}

	for(i=0; i<NUM_WAVE_HEADERS; i++)
	{
		m_wavfiledata[i] = (char*) malloc(FILE_READ_CHUNK_SIZE);
		if(!m_wavfiledata[i])
			goto Exit1;
	}

	fp.close();

	wfx.cbSize = sizeof(WAVEFORMATEX);
	wfx.wFormatTag = wfh.wFormatTag;
	wfx.nChannels = wfh.nChannels;
	wfx.nSamplesPerSec = wfh.nSamplesPerSec;
	wfx.wBitsPerSample = wfh.wBitsPerSample;
	wfx.nBlockAlign = wfh.nBlockAlign;
	wfx.nAvgBytesPerSec = wfh.nAvgBytesPerSec;

	for(uDeviceID=0; uDeviceID<num_audio_dev; uDeviceID++)
	{
		res = waveOutOpen(&m_hwo, uDeviceID, &wfx, (DWORD)waveOutProc, (DWORD)this, CALLBACK_FUNCTION);
		if(res!=MMSYSERR_NOERROR) 
		{
			if(res==WAVERR_BADFORMAT)
				int xx=0;
			continue;
		}
		break;
	}
	
	if(!m_hwo && uDeviceID==num_audio_dev)
		goto Exit1;

	m_play_data_arr = (PLAY_DATA *)malloc(sizeof(PLAY_DATA)*NUM_WAVE_HEADERS);
	if(!m_play_data_arr)
		goto Exit1;

	for(i=0; i<NUM_WAVE_HEADERS; i++)
	{
		wh[i]=(WAVEHDR*)malloc(sizeof(WAVEHDR));
		if(!wh[i])
		{
			goto Exit1;
		}
		memset(wh[i], 0, sizeof(WAVEHDR));
		wh[i]->lpData = m_wavfiledata[i];
		wh[i]->dwBufferLength = FILE_READ_CHUNK_SIZE;

		res = ::waveOutPrepareHeader(m_hwo, wh[i], sizeof(WAVEHDR));
		if(res!=MMSYSERR_NOERROR) 
			goto Exit1;

		//SetTimer(m_hWnd, (unsigned int)wh[i], (unsigned int)0, (TIMERPROC)WaveStart);
		//::waveOutWrite(m_hwo, wh[i], sizeof(WAVEHDR));
		//play_data = (PLAY_DATA*) malloc(sizeof(PLAY_DATA));
		play_data = GetPlayData(i);
		if(play_data) 
		{
			play_data->pwh = (WAVEHDR*)wh[i];
			play_data->wav = (WAV*) this;
			//play_data->proc = (PLAYDATAPROC)PlayData;
			play_data->n = i;
			//PlayMore(play_data);
		}
	}

	memcpy(&m_wfh, &wfh, sizeof(wfh));
	memcpy(&m_wfx, &wfx, sizeof(wfx));
	return 1;
Exit1:
	for(i=0; i<NUM_WAVE_HEADERS; i++)
	{
		if(wh[i]) 
			free(wh[i]);
		if(m_wavfiledata[i]) 
			free(m_wavfiledata[i]);
		m_wavfiledata[i]=0;
	}
	fp.close();

	return 0;
#endif
}

void WAV::PlayStart()
{
	int i;
	PLAY_DATA *play_data = 0;
	for(i=0; i<NUM_WAVE_HEADERS; i++)
	{
		play_data = GetPlayData(i);
		PlayMore(play_data);
	}
}

unsigned int WAV::WriteFile(const char* wavfile, unsigned int num_samples_to_write, int* in_samples, unsigned int write_bits_per_sample, WAVEFILEHEADER *wfh)
{
#if 0
	FILEIO fp;
	unsigned int i, max=0, ret=0;
	if(wfh)
	{
		ret = fp.open(wavfile, FILEIO_MODE_WRITE|FILEIO_MODE_CREATE_ALWAYS);
		if(!ret)
			return 0;
		ret = 0;
		ret += fp.write(wfh, sizeof(WAVEFILEHEADER));
	}
	else
	{
		ret = fp.open(wavfile, FILEIO_MODE_WRITE|FILEIO_MODE_OPEN_EXISTING);
		if(!ret)
			return 0;
		ret = 0;
	}
	int diff = 1;
	int k;
	for(k=0; k<(int)(write_bits_per_sample-1); k++)
		diff<<=1;
/*	for(i=0; i<num_samples_to_write; i++)
	{
		if(in_samples[i]<0)
			absval=-in_samples[i];
		else
			absval=in_samples[i];

		if(absval>max)
			max=absval;
	}
*/	for(i=0; i<num_samples_to_write; i++)
	{
		//in_samples[i] = in_samples[i] * 127 / max;
		if(in_samples[i]>0)
			in_samples[i]-=(diff-1);
		else if(in_samples[i]<=0)
			in_samples[i]+=diff;
		if(in_samples[i]>(diff-1))
			in_samples[i] = (diff-1);
		else if(in_samples[i]<-diff)
			in_samples[i] = -diff;
		char ch = (char)in_samples[i];
		ret += fp.write((void*)&ch, 1);
	}
	fp.close();
	return ret;
#endif
}

unsigned int WAV::ReadFile(const char* wavfile, unsigned int start_pos, unsigned int num_samples_to_read, int* out_samples)
{	
#if 0
	WAVEFILEHEADER wfh;
	WAVEFORMATEX wfx;
	FILEIO fp;
	int filepos=0;
	unsigned int i;
	unsigned int num_bytes_read;
	unsigned int bytes_per_sample;
	unsigned int num_samples_read=0;
	char* y=0;
	int diff = 1;
	int k;

	if(!wavfile 
		|| !num_samples_to_read 
		|| !out_samples 
		|| !fp.open(wavfile, FILEIO_MODE_READ|FILEIO_MODE_OPEN_EXISTING) 
		)
		goto Exit1;
	filepos=fp.read(&wfh, sizeof(WAVEFILEHEADER));

/*
	FILEIO fp2;
	fp2.open("wfh.wav", FILEIO_MODE_WRITE|FILEIO_MODE_CREATE_NEW);
	fp2.write(&wfh, sizeof(WAVEFILEHEADER));
	fp2.close();
*/
	wfx.cbSize = sizeof(WAVEFORMATEX);
	wfx.wFormatTag = wfh.wFormatTag;
	wfx.nChannels = wfh.nChannels;
	wfx.nSamplesPerSec = wfh.nSamplesPerSec;
	wfx.wBitsPerSample = wfh.wBitsPerSample;
	wfx.nBlockAlign = wfh.nBlockAlign;
	wfx.nAvgBytesPerSec = wfh.nAvgBytesPerSec;

	bytes_per_sample = wfh.wBitsPerSample/8;
	for(k=0; k<(wfh.wBitsPerSample-1); k++)
		diff<<=1;

	y=(char*) malloc(num_samples_to_read * bytes_per_sample);
	if(y)
	{
		fp.seek(filepos+start_pos, FILEIO_BEGIN);
		num_bytes_read=fp.read(y, num_samples_to_read * bytes_per_sample);
		fp.close();
		num_samples_read = num_bytes_read / bytes_per_sample;
		for(i=0; i<num_samples_read; i++)
		{
			if(y[i]<0)
				y[i]+=(diff-1);
			else if(y[i]>=0)
				y[i]-=diff;
			out_samples[i]=(int)y[i];
		}
		free(y);
	}
	else
		fp.close();
	return num_samples_read;

Exit1:
	fp.close();

	return 0;
#endif
}

//This will create sine waves upto max_num_waves or seconds, whichever is shorter.
//Filter is defined as one wave. When one wave is complete, *num_samples_in_filter is copied.
//The wave(s) are stored in wavfile
unsigned int WAV::CreateSineWav(char* wavfile, unsigned char max_amplitude, unsigned int hertz, unsigned int seconds,
			unsigned int max_num_waves, unsigned int *num_samples_in_filter, unsigned int *out_bytes_per_sample)
{
#if 0
	unsigned int ret=0;
	WAVEFILEHEADER wfh;
	FILEIO fp;
	fp.open(TXT("wfh.wav"), FILEIO_MODE_READ|FILEIO_MODE_OPEN_EXISTING);
	fp.read(&wfh, sizeof(WAVEFILEHEADER));
	fp.close();

	unsigned int bytes_per_sample = wfh.wBitsPerSample/8;
	unsigned int sample_rate = wfh.nSamplesPerSec;

	int diff = 1;
	int k;
	for(k=0; k<(wfh.wBitsPerSample-1); k++)
		diff<<=1;

	unsigned int i;
	float pi = (float)3.141592653;
	float t = (float)sample_rate, y;
	unsigned int num_samples = (int)(t*seconds);
	unsigned int num_waves = 1;
	int* data = (int*) malloc(num_samples*bytes_per_sample*sizeof(int));
	if(!data)
		return 0;
	memset(data, 0, num_samples*bytes_per_sample*sizeof(int));
	for(i=0; i<num_samples; i++)
	{
		float theta = 2*pi*hertz*i/t;
		y = max_amplitude*(float)sin(theta);
		data[i] = (int)y;

		if(theta >= 2*pi*num_waves)
		{
			if(num_waves==1)
			{
				if(num_samples_in_filter)
					*num_samples_in_filter = i;
				if(out_bytes_per_sample)
					*out_bytes_per_sample = bytes_per_sample;
			}
			if(num_waves==max_num_waves)
				break;
			num_waves++;
		}
	}

	WriteFile(wavfile, i, data, bytes_per_sample*8, &wfh);

	free(data);
	return ret;
#endif
}

/*******************/


void 
WriteWaveFileHeader(FILE *fp, int in_channels, int in_sample_rate, int in_bits_per_sample, int in_pcm_data_bytes)
{
	WAVEFILEHEADER *pwf, wfh;
	pwf = &wfh;

	memcpy((char*)&pwf->ChunkID, "RIFF", 4);
	memcpy((char*)&pwf->Format, "WAVE", 4);
	memcpy((char*)&pwf->Subchunk1ID, "fmt ", 4);
	pwf->Subchunk1Size = 16;
	pwf->wFormatTag = WAVE_FORMAT_PCM;
	pwf->nChannels = in_channels;
	pwf->nSamplesPerSec = in_sample_rate;
	pwf->wBitsPerSample = in_bits_per_sample;
	pwf->nBlockAlign = (in_channels * in_bits_per_sample)/8;
	pwf->nAvgBytesPerSec = pwf->nSamplesPerSec * pwf->nBlockAlign;
	memcpy((char*)&pwf->Subchunk2ID, "data", 4);
	pwf->Subchunk2Size = in_pcm_data_bytes;
	pwf->ChunkSize = 36+pwf->Subchunk2Size;

	fwrite((char*)pwf, 1, sizeof(WAVEFILEHEADER), fp);
}

void InitWaveFileHeader(WAVEFILEHEADER *pwf, WAVEFORMATEX* pwfx, DWORD in_pcm_data_bytes)
{
	memcpy((char*)&pwf->ChunkID, "RIFF", 4);
	pwf->ChunkSize = 0;
	memcpy((char*)&pwf->Format, "WAVE", 4);
	memcpy((char*)&pwf->Subchunk1ID, "fmt ", 4);
	pwf->Subchunk1Size = 16;
	pwf->wFormatTag = pwfx->wFormatTag;
	pwf->nChannels = pwfx->nChannels;
	pwf->nSamplesPerSec = pwfx->nSamplesPerSec;
	pwf->nAvgBytesPerSec = pwfx->nAvgBytesPerSec;
	pwf->nBlockAlign = pwfx->nBlockAlign;
	pwf->wBitsPerSample = pwfx->wBitsPerSample;
	memcpy((char*)&pwf->Subchunk2ID, "data", 4);
	pwf->Subchunk2Size = in_pcm_data_bytes;
}
