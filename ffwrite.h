#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/mathematics.h"
#include "libavutil/opt.h"
#include "cmdutils.h"

typedef struct _MY_ENCODER_DATA
{
    AVFormatContext *oc;
    AVStream *audio_st;
    AVStream *video_st;
    AVOutputFormat *fmt;
    AVCodec *audio_codec;
    AVCodec *video_codec;
    enum AVCodecID audio_codec_id;
    enum AVCodecID video_codec_id;
    int64_t audio_pts;
    int64_t video_pts;
    struct timeval beginning;
    int64_t start_time;
    int width;
    int height;
    int frames_per_second;
    int audio_bitrate;
    int audio_sample_rate;
    int audio_num_channels;
    int audio_frame_size;
    int io_buffer_sz;
    unsigned char *io_buffer;
    void *opaque_io_data;
    int (*read_func)(void *opaque, uint8_t *buf, int buf_size);
    int (*write_func)(void *opaque, uint8_t *buf, int buf_size);
    int64_t (*seek_func)(void *opaque, int64_t offset, int whence);
}
MyEncoder;

int initializeEncoder(const char *file_name, unsigned char record_audio, MyEncoder *e);
int writePacket(AVFormatContext *oc, AVStream *st, AVPacket *pkt);
void writeVideoFrame(MyEncoder *e, AVFrame *frame);
void writeAudio(MyEncoder *e, AVFrame *frame);
void deinitializeEncoder(MyEncoder *e);
AVFrame *allocFrame(int width, int height);
void freeFrame(AVFrame *frame);
int64_t getTickCount();
int64_t timeElapsed(struct timeval *beginning);
int64_t calculateFrameTimestamp(uint64_t start_time, AVStream *st);
AVRational timebase(AVStream *st);
AVFrame *enqueAudioSamples(AVStream *st, int64_t pts,
                       void *audio_samples_p, unsigned long num_samples);

#ifdef __cplusplus
}
#endif
