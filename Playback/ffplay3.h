#ifndef MAIN3_H
#define MAIN3_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavutil/mathematics.h"

#include "libavutil/opt.h"
#include "cmdutils.h"


typedef int (*READ_STREAM_CB)(void *opaque, uint8_t *buf, int buf_size);
typedef int64_t (*SEEK_STREAM_CB)(void *opaque, int64_t offset, int whence);
typedef int (*GOT_FRAME_CB)(void* user, enum AVMediaType media_type, AVFrame *frame, int64_t dts, int pkt_size);

typedef struct Info {
    int width;
    int height;
    int64_t duration;
    int64_t duration_ms;
    int64_t num_frames;
    int frame_rate;
}Info;

typedef struct Decoder {
    int packet_pending;
    int64_t start_pts, next_pts;
    AVRational start_pts_tb, next_pts_tb;
    AVPacket pkt_temp, flush_pkt;
    AVFormatContext *ic;
    AVCodecContext *avctx[AVMEDIA_TYPE_NB];
    int st_index[AVMEDIA_TYPE_NB];
    AVStream *stream[AVMEDIA_TYPE_NB];
    AVDictionary *format_opts1;
    AVDictionary *codec_opts1;
    struct Info info;
    const char* filename;
    int buffer_size_;
    unsigned char * buffer_;
    int fp_;
    struct sockaddr_in sa_;
    GOT_FRAME_CB gotFrameCb;
    READ_STREAM_CB readfn_;
    SEEK_STREAM_CB seekfn_;
    void* user_data;
}Decoder;

void decoder_register();
int decoder_open(Decoder *d);
void decoder_play(Decoder *d, int seek_ms);
void decoder_close(Decoder *d);

#ifdef __cplusplus
}
#endif
#endif // MAIN3_H

