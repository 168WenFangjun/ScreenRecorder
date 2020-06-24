#pragma once

#include <stdio.h>
#include <stdint.h>
#include <QString>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef long long int64_t;

struct x264_t;
struct x264_picture_t;
struct x264_nal_t;
struct AVRational;
struct AVCodecContext;
struct AVOutputFormat;
struct AVFormatContext;
struct AVCodec;
struct AVStream;
class X264Encoder
{
public:
    X264Encoder(AVFormatContext *oc, AVStream *video_st);
    virtual ~X264Encoder();
    bool init(int width, int height, AVRational &timebase, int fps);
    void encode(uint8_t *plane[4], int64_t pts);
    bool flush();

protected:
    x264_t* m_encoder;
    x264_picture_t *m_pic_in;
    int m_width;
    int m_height;
    FILE* m_h264_fp;
    x264_nal_t* m_nals;
    int m_i_nals;
    QString m_out_dir;
//    AVOutputFormat *m_fmt;
    AVFormatContext *m_oc;
//    AVCodec *m_audio_codec;
//    AVCodec *m_video_codec;
    AVStream *m_video_st;
    uint8_t* m_plane[3];

    void writeH264Frame(uint8_t *data, int size);
    void finish();
};
