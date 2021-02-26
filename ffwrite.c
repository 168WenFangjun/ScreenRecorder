#include <sys/time.h>

#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavresample/resample.h>
#include "ffwrite.h"

extern void mylog(const char *fmt, ...);

const int program_birth_year = 2016;
const char program_name[32] = "test";

void show_help_default(const char *opt, const char *arg)
{
}

/*
 * Copyright (c) 2003 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

/* 5 seconds stream duration */
#define STREAM_DURATION   200.0
#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_NB_FRAMES  ((int)(STREAM_DURATION * STREAM_FRAME_RATE))
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */

static int sws_flags = SWS_BICUBIC;

/**************************************************************/
/* audio output */

static float t, tincr, tincr2;
static int16_t *samples;
static int audio_input_frame_size;

/* Add an output stream. */
static AVStream *add_stream(AVFormatContext *oc, AVCodec **codec,
                            enum AVCodecID codec_id)
{
    AVCodecContext *c;
    AVStream *st;

    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        fprintf(stderr, "Could not find codec\n");
        exit(1);
    }

    st = avformat_new_stream(oc, *codec);
    if (!st) {
        fprintf(stderr, "Could not allocate stream\n");
        exit(1);
    }
    st->id = oc->nb_streams-1;
    c = st->codec;

    switch ((*codec)->type) {
    case AVMEDIA_TYPE_AUDIO:
        st->id = 1;
        c->sample_fmt  = AV_SAMPLE_FMT_FLTP;//fltp: floats in planar format (c0 c0 c0 c1 c1 c1), vs flt: floats in interleaved format (c0 c1 c0 c1 c0 c1)
        c->bit_rate    = 64000;
        c->sample_rate = 44100;
        c->channels    = 1;
        break;

    case AVMEDIA_TYPE_VIDEO:
        avcodec_get_context_defaults3(c, *codec);
        c->codec_id = codec_id;

        c->bit_rate = 400000;
        /* Resolution must be a multiple of two. */
        c->width    = 0;//352;
        c->height   = 0;//288;
        /* timebase: This is the fundamental unit of time (in seconds) in terms
         * of which frame timestamps are represented. For fixed-fps content,
         * timebase should be 1/framerate and timestamp increments should be
         * identical to 1. */
        st->time_base.den = STREAM_FRAME_RATE;
        st->time_base.num = 1;
        c->time_base.den = STREAM_FRAME_RATE;
        c->time_base.num = 1;
        c->gop_size      = 12; /* emit one intra frame every twelve frames at most */
        c->pix_fmt       = STREAM_PIX_FMT;
        switch(c->codec_id)
        {
        case AV_CODEC_ID_MPEG2VIDEO: {
            /* just for testing, we also add B frames */
            c->max_b_frames = 2;
            break;
        }
        case AV_CODEC_ID_MPEG1VIDEO: {
            /* Needed to avoid using macroblocks in which some coeffs overflow.
             * This does not happen with normal video, it just happens here as
             * the motion of the chroma plane does not match the luma plane. */
            c->mb_decision = 2;
            break;
        }
        case AV_CODEC_ID_VP8:
        case AV_CODEC_ID_VP9: {
//            VPxEncoderContext *ctx = (VPxEncoderContext *)c->priv_data;
            break;
        }
        }//switch(c->codec_id)
    break;

    default:
        break;
    }

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    return st;
}

/**************************************************************/
/* audio output */

static float t, tincr, tincr2;
static int16_t *samples;
static int audio_input_frame_size;

static void open_audio(AVFormatContext *oc, AVCodec *codec, AVStream *st)
{
    AVCodecContext *c;

    c = st->codec;
    /* open it */
    if (avcodec_open2(c, codec, NULL) < 0) {
        fprintf(stderr, "Could not open audio codec\n");
        exit(1);
    }

    /* init signal generator */
    t     = 0;
    tincr = 2 * M_PI * 110.0 / c->sample_rate;
    /* increment frequency by 110 Hz per second */
    tincr2 = 2 * M_PI * 110.0 / c->sample_rate / c->sample_rate;

    if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
        audio_input_frame_size = 10000;
    else
        audio_input_frame_size = c->frame_size;
    samples = av_malloc(audio_input_frame_size *
                        av_get_bytes_per_sample(c->sample_fmt) *
                        c->channels);
    if (!samples) {
        fprintf(stderr, "Could not allocate audio samples buffer\n");
        exit(1);
    }
}

/* Prepare a 16 bit dummy audio frame of 'frame_size' samples and
 * 'nb_channels' channels. */
static void get_audio_frame(int16_t *samples, int frame_size, int nb_channels)
{
    int j, i, v;
    int16_t *q;

    q = samples;
    for (j = 0; j < frame_size; j++) {
        v = (int)(sin(t) * 10000);
        for (i = 0; i < nb_channels; i++)
            *q++ = v;
        t     += tincr;
        tincr += tincr2;
    }
}

static uint8_t* resampleSigned16ToFloat(AVCodecContext *codec, uint8_t *audio_samples, int *nb_samples_p)
{
    int nb_samples = *nb_samples_p;
    float* resamples = (float *)malloc(nb_samples * codec->channels * sizeof(float));
    memset(resamples, 0, nb_samples * codec->channels * sizeof(float));
    float min = -32768.;
    float max = 32767.;
    short* input = (short*)audio_samples;
    for(int i = 0; i < nb_samples * codec->channels; ++i)
    {
        resamples[i] = (input[i] - min) / (max - min) * (1 - - 1) - 1;
    }
    return resamples;
}

static void write_audio_frame2(AVFormatContext *oc, AVStream *st, AVFrame *frame)
{

    AVPacket pkt = { 0 }; // data and size must be 0;
    int got_packet = 0;

    av_init_packet(&pkt);
    int ret = avcodec_encode_audio2(st->codec, &pkt, frame, &got_packet);
    if (ret < 0)
    {
        fprintf(stderr, "Error encoding audio frame\n");
//        exit(1);
        return;
    }

    if(got_packet == 1)
    {
        pkt.stream_index = st->index;
        pkt.pts = pkt.dts = frame->pts;

        /* Write the compressed frame to the media file. */
        if (av_interleaved_write_frame(oc, &pkt) != 0)
        {
            fprintf(stderr, "Error while writing audio frame\n");
            exit(1);
        }
    }
    else
    {
//        fprintf(stderr, "got_packet = 0\n");
    }
}

static void write_audio_frame(AVFormatContext *oc, AVStream *st)
{
    AVCodecContext *c;
    AVPacket pkt = { 0 }; // data and size must be 0;
    AVFrame *frame = av_frame_alloc();
    int got_packet, ret;

    av_init_packet(&pkt);
    c = st->codec;

    get_audio_frame(samples, audio_input_frame_size, c->channels);
    frame->nb_samples = audio_input_frame_size;
    avcodec_fill_audio_frame(frame, c->channels, c->sample_fmt,
                             (uint8_t *)samples,
                             audio_input_frame_size *
                             av_get_bytes_per_sample(c->sample_fmt) *
                             c->channels, 1);

    ret = avcodec_encode_audio2(c, &pkt, frame, &got_packet);
    if (ret < 0) {
        fprintf(stderr, "Error encoding audio frame\n");
        exit(1);
    }

    if (got_packet)
    {
        pkt.stream_index = st->index;

        /* Write the compressed frame to the media file. */
        if (av_interleaved_write_frame(oc, &pkt) != 0) {
            fprintf(stderr, "Error while writing audio frame\n");
            exit(1);
        }
    }
    av_frame_free(&frame);
}

static void close_audio(AVFormatContext *oc, AVStream *st)
{
    avcodec_close(st->codec);

    av_free(samples);
}

/**************************************************************/
/* video output */

static AVFrame *frame;
static AVPicture src_picture, dst_picture;
static int frame_count;

static void open_video(AVFormatContext *oc, AVCodec *codec, AVStream *st)
{
    int ret;
    AVCodecContext *c = st->codec;

    /* open the codec */
    int err = avcodec_open2(c, codec, NULL);
    if (err < 0) {
        fprintf(stderr, "Could not open video codec\n");
        exit(1);
    }

    /* allocate and init a re-usable frame */
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    /* Allocate the encoded raw picture. */
    ret = avpicture_alloc(&dst_picture, c->pix_fmt, c->width, c->height);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate picture\n");
        exit(1);
    }

    /* If the output format is not YUV420P, then a temporary YUV420P
     * picture is needed too. It is then converted to the required
     * output format. */
    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        ret = avpicture_alloc(&src_picture, AV_PIX_FMT_YUV420P, c->width, c->height);
        if (ret < 0) {
            fprintf(stderr, "Could not allocate temporary picture\n");
            exit(1);
        }
    }

    /* copy data and linesize picture pointers to frame */
    *((AVPicture *)frame) = dst_picture;
}

/* Prepare a dummy image. */
static void fill_yuv_image(AVPicture *pict, int frame_index,
                           int width, int height)
{
    int x, y, i;

    i = frame_index;

    /* Y */
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            pict->data[0][y * pict->linesize[0] + x] = x + y + i * 3;

    /* Cb and Cr */
    for (y = 0; y < height / 2; y++) {
        for (x = 0; x < width / 2; x++) {
            pict->data[1][y * pict->linesize[1] + x] = 128 + y + i * 2;
            pict->data[2][y * pict->linesize[2] + x] = 64 + x + i * 5;
        }
    }
}

AVFrame *enqueAudioSamples(AVStream *st,
                           int64_t pts,
                       void *audio_samples_p,
                       unsigned long num_samples)
{
    audio_samples_p = resampleSigned16ToFloat(st->codec, audio_samples_p, &num_samples);
    AVFrame *frame = av_frame_alloc();

    frame->format = st->codec->sample_fmt;
    frame->nb_samples = num_samples;
    frame->channels = st->codec->channels;
    frame->channel_layout = st->codec->channel_layout;
    frame->sample_rate = st->codec->sample_rate;
    frame->pts = pts;

    int bytes_per_sample = av_get_bytes_per_sample(st->codec->sample_fmt);
    int ret = avcodec_fill_audio_frame(frame, st->codec->channels, st->codec->sample_fmt,
                                       audio_samples_p,
                                       frame->nb_samples *
                                       bytes_per_sample *
                                       st->codec->channels,
                                       1);
    if (ret < 0)
    {
        fprintf(stderr, "Error filling audio frame\n");
        exit(1);
    }

    free(audio_samples_p);
    return frame;
}

int writePacket(AVFormatContext *oc, AVStream *st, AVPacket *pkt)
{
    AVCodecContext *avctx = st->codec;

    if (avctx->coded_frame->key_frame)
    {
        pkt->flags |= AV_PKT_FLAG_KEY;
    }
    pkt->stream_index = st->index;

    /* Write the compressed frame to the media file. */
    return av_interleaved_write_frame(oc, pkt);
}

static void write_video_frame2(AVFormatContext *oc, AVStream *st, AVFrame *frame)
{
    /* encode the image */
    int ret;
    AVPacket pkt;
    AVCodecContext *avctx = st->codec;

    ret = avcodec_send_frame(avctx, frame);
    if (ret < 0)
    {
        fprintf(stderr, "Error encoding video frame\n");
        exit(1);
    }

    av_init_packet(&pkt);
    pkt.data = NULL;    // packet data will be allocated by the encoder
    pkt.size = 0;

    ret = avcodec_receive_packet(avctx, &pkt);
    if(ret < 0)
    {
        return;
    }

    writePacket(oc, st, &pkt);
}

static void write_video_frame(AVFormatContext *oc, AVStream *st)
{
    int ret;
    static struct SwsContext *sws_ctx;
    AVCodecContext *c = st->codec;

    if (frame_count >= STREAM_NB_FRAMES) {
        /* No more frames to compress. The codec has a latency of a few
         * frames if using B-frames, so we get the last frames by
         * passing the same picture again. */
    } else {
        if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
            /* as we only generate a YUV420P picture, we must convert it
             * to the codec pixel format if needed */
            if (!sws_ctx) {
                sws_ctx = sws_getContext(c->width, c->height, AV_PIX_FMT_YUV420P,
                                         c->width, c->height, c->pix_fmt,
                                         sws_flags, NULL, NULL, NULL);
                if (!sws_ctx) {
                    fprintf(stderr,
                            "Could not initialize the conversion context\n");
                    exit(1);
                }
            }
            fill_yuv_image(&src_picture, frame_count, c->width, c->height);
            sws_scale(sws_ctx,
                      (const uint8_t * const *)src_picture.data, src_picture.linesize,
                      0, c->height, dst_picture.data, dst_picture.linesize);
        } else {
            fill_yuv_image(&dst_picture, frame_count, c->width, c->height);
        }
    }

    if (oc->oformat->flags & AVFMT_NOFILE) {
        /* Raw video case - directly store the picture in the packet */
        AVPacket pkt;
        av_init_packet(&pkt);

        pkt.flags        |= AV_PKT_FLAG_KEY;
        pkt.stream_index  = st->index;
        pkt.data          = dst_picture.data[0];
        pkt.size          = sizeof(AVPicture);

        ret = av_interleaved_write_frame(oc, &pkt);
    } else {
        /* encode the image */
        AVPacket pkt;
        int got_output;

        av_init_packet(&pkt);
        pkt.data = NULL;    // packet data will be allocated by the encoder
        pkt.size = 0;

        ret = avcodec_encode_video2(c, &pkt, frame, &got_output);
        if (ret < 0) {
            fprintf(stderr, "Error encoding video frame\n");
            exit(1);
        }

        /* If size is zero, it means the image was buffered. */
        if (got_output) {
            if (c->coded_frame->key_frame)
                pkt.flags |= AV_PKT_FLAG_KEY;

            pkt.stream_index = st->index;

            /* Write the compressed frame to the media file. */
            ret = av_interleaved_write_frame(oc, &pkt);
        } else {
            ret = 0;
        }
    }
    if (ret != 0) {
        fprintf(stderr, "Error while writing video frame\n");
        exit(1);
    }
    frame_count++;
}

static void close_video(AVFormatContext *oc, AVStream *st)
{
    avcodec_close(st->codec);
    av_free(src_picture.data[0]);
    av_free(dst_picture.data[0]);
    av_free(frame);
}

/**************************************************************/
/* media file output */


int ffwrite(const char *filename)
{
    AVOutputFormat *fmt;
    AVFormatContext *oc;
    AVStream *audio_st, *video_st;
    AVCodec *audio_codec, *video_codec;
    double audio_pts, video_pts;
    int i;
    int pts = 0;

    /* Initialize libavcodec, and register all codecs and formats. */
    av_register_all();

    /* allocate the output media context */
    avformat_alloc_output_context2(&oc, NULL, NULL, filename);
    if (!oc) {
        printf("Could not deduce output format from file extension: using MPEG.\n");
        avformat_alloc_output_context2(&oc, NULL, "mpeg", filename);
    }
    if (!oc) {
        return 1;
    }
    fmt = oc->oformat;

    /* Add the audio and video streams using the default format codecs
     * and initialize the codecs. */
    video_st = NULL;
    audio_st = NULL;

    if (fmt->video_codec != AV_CODEC_ID_NONE) {
        video_st = add_stream(oc, &video_codec, fmt->video_codec);
        video_st->codec->width = 1024;
        video_st->codec->height = 768;
    }
//    if (fmt->audio_codec != AV_CODEC_ID_NONE) {
//        audio_st = add_stream(oc, &audio_codec, fmt->audio_codec);
//    }

    /* Now that all the parameters are set, we can open the audio and
     * video codecs and allocate the necessary encode buffers. */
    if (video_st)
    {
        open_video(oc, video_codec, video_st);
    }
    if (audio_st)
    {
        open_audio(oc, audio_codec, audio_st);
    }

    av_dump_format(oc, 0, filename, 1);

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        if (avio_open(&oc->pb, filename, AVIO_FLAG_WRITE) < 0) {
            fprintf(stderr, "Could not open '%s'\n", filename);
            return 1;
        }
    }

    /* Write the stream header, if any. */
    if (avformat_write_header(oc, NULL) < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        return 1;
    }

    if (frame)
        frame->pts = 0;
    for (;;) {
        /* Compute current audio and video time. */
        if (audio_st)
            audio_pts = (double)av_stream_get_end_pts(audio_st) * audio_st->time_base.num / audio_st->time_base.den;
        else
            audio_pts = 0.0;

        if (video_st)
            video_pts = (double)av_stream_get_end_pts(video_st) * video_st->time_base.num /
                        video_st->time_base.den;
        else
            video_pts = 0.0;

        if ((!audio_st || audio_pts >= STREAM_DURATION) &&
            (!video_st || video_pts >= STREAM_DURATION))
            break;

        /* write interleaved audio and video frames */
        if (!video_st || (video_st && audio_st && audio_pts < video_pts)) {
            write_audio_frame(oc, audio_st);
        } else {
            write_video_frame(oc, video_st);
            pts += av_rescale_q(1, video_st->codec->time_base, video_st->time_base);
        }
    }

    /* Write the trailer, if any. The trailer must be written before you
     * close the CodecContexts open when you wrote the header; otherwise
     * av_write_trailer() may try to use memory that was freed on
     * av_codec_close(). */
    av_write_trailer(oc);

    /* Close each codec. */
    if (video_st)
        close_video(oc, video_st);
    if (audio_st)
        close_audio(oc, audio_st);

    /* Free the streams. */
    for (i = 0; i < oc->nb_streams; i++) {
        av_freep(&oc->streams[i]->codec);
        av_freep(&oc->streams[i]);
    }

    if (!(fmt->flags & AVFMT_NOFILE))
        /* Close the output file. */
        avio_close(oc->pb);

    /* free the stream */
    av_free(oc);

    return 0;
}

/*
 * */

int64_t getTickCount()
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec * 1000) + (now.tv_nsec / 1000000);
}

int64_t timeElapsed(struct timeval *beginning)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    int64_t diff = (now.tv_sec - beginning->tv_sec) * 1000 +
            (now.tv_usec - beginning->tv_usec) / 1000000;
    return diff;
}

AVRational timebase(AVStream *st)
{
    int64_t time_unit = av_rescale_q(1, st->codec->time_base, st->time_base);
    AVRational ret;
    ret.num = (time_unit * st->codec->time_base.den);
    ret.den = 1000;
    return ret;
}
int64_t calculateFrameTimestamp(uint64_t start_time, AVStream *st)
{
    int64_t time_elapsed = (int64_t)(getTickCount() - start_time);
    AVRational time_base = timebase(st);
    return (time_elapsed * time_base.num) / time_base.den;
}

static void customIo(MyEncoder *e)
{
    e->io_buffer_sz = 256 * 1024;
    e->io_buffer = (unsigned char*)malloc(e->io_buffer_sz);
    e->oc->pb = avio_alloc_context(e->io_buffer,        // internal Buffer
                                   e->io_buffer_sz,     // and its size
                                   1,                   // bWriteable (1=true,0=false)
                                   e->opaque_io_data,   // user data ; will be passed to our callback functions
                                   0,                   // ReadFunc
                                   e->write_func,       // Write callback function
                                   e->seek_func);
    e->oc->flags = AVFMT_FLAG_CUSTOM_IO;
    e->oc->oformat->flags |= AVFMT_NOFILE;
}

int initializeEncoder(const char *file_name, unsigned char record_audio, MyEncoder *e)
{
    e->video_pts = 0;
    e->audio_pts = 0;
    e->start_time = getTickCount();
    gettimeofday(&e->beginning, NULL);

    /* Initialize libavcodec, and register all codecs and formats. */
    av_register_all();

    /* allocate the output media context */
    avformat_alloc_output_context2(&e->oc, NULL, NULL, file_name);
    if (!e->oc)
    {
        printf("Could not deduce output format from file extension: using MPEG.\n");
        avformat_alloc_output_context2(&e->oc, NULL, "mpeg", file_name);
    }
    if (!e->oc)
    {
        return 0;
    }

    e->oc->oformat->flags &= ~AVFMT_NOFILE;
    customIo(e);
    e->fmt = e->oc->oformat;

    /* Add the audio and video streams using the default format codecs
     * and initialize the codecs. */
    e->video_st = NULL;
    e->audio_st = NULL;
    e->fmt->video_codec = e->video_codec_id;
    e->fmt->audio_codec = e->audio_codec_id;

    if (e->fmt->video_codec != AV_CODEC_ID_NONE)
    {
        e->video_st = add_stream(e->oc, &e->video_codec, e->fmt->video_codec);
        e->video_st->codec->width = e->width;
        e->video_st->codec->height = e->height;
        e->video_st->time_base.den = e->frames_per_second;
        e->video_st->codec->codec_id = e->fmt->video_codec;
        e->video_st->codec->flags |= AV_CODEC_FLAG_QSCALE;
        e->video_st->codec->global_quality = 51*FF_QP2LAMBDA;//https://superuser.com/questions/638069/what-is-the-maximum-and-minimum-of-q-value-in-ffmpeg
    }
    if (e->fmt->audio_codec != AV_CODEC_ID_NONE && record_audio)
    {
        e->audio_st = add_stream(e->oc, &e->audio_codec, e->fmt->audio_codec);
        e->audio_st->codec->bit_rate = e->audio_bitrate;
        e->audio_st->codec->sample_rate = e->audio_sample_rate;
        e->audio_st->codec->channels    = e->audio_num_channels;
        e->audio_st->codec->frame_size = e->audio_frame_size;
        e->audio_st->codec->profile = FF_PROFILE_AAC_MAIN;
        e->audio_st->codec->codec_id = e->fmt->audio_codec;
        e->audio_st->codec->codec_type = AVMEDIA_TYPE_AUDIO;
        e->audio_st->codec->sample_fmt = AV_SAMPLE_FMT_FLTP;
        e->audio_st->codec->channels = e->audio_num_channels;
        e->audio_st->codec->channel_layout = av_get_channel_layout( e->audio_num_channels == 1 ? "mono" : "stereo" );
        e->audio_st->codec->time_base.num = 1;
        e->audio_st->codec->time_base.den = e->audio_sample_rate;
        e->audio_st->time_base.num = 1;
        e->audio_st->time_base.den = e->frames_per_second;
//        e->audio_st->codec->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    }

    /* Now that all the parameters are set, we can open the audio and
     * video codecs and allocate the necessary encode buffers. */
    if (e->video_st)
    {
        open_video(e->oc, e->video_codec, e->video_st);
    }
    if (e->audio_st)
    {
        open_audio(e->oc, e->audio_codec, e->audio_st);
    }

    av_dump_format(e->oc, 0, file_name, 1);

    /* open the output file, if needed */
    if (!(e->fmt->flags & AVFMT_NOFILE))
    {
        if (avio_open(&e->oc->pb, file_name, AVIO_FLAG_WRITE) < 0)
        {
            fprintf(stderr, "Could not open '%s'\n", file_name);
            return 0;
        }
    }

    /* Write the stream header, if any. */
    if (avformat_write_header(e->oc, NULL) < 0)
    {
        fprintf(stderr, "Error occurred when opening output file\n");
        return 0;
    }

    return 1;
}

void deinitializeEncoder(MyEncoder *e)
{
    AVFormatContext *oc = e->oc;
    AVStream *audio_st = e->audio_st;
    AVStream *video_st = e->video_st;
    AVOutputFormat *fmt = e->fmt;

    /* Write the trailer, if any. The trailer must be written before you
     * close the CodecContexts open when you wrote the header; otherwise
     * av_write_trailer() may try to use memory that was freed on
     * av_codec_close(). */
    av_write_trailer(oc);

    /* Close each codec. */
    if (video_st)
        close_video(oc, video_st);
    if (audio_st)
        close_audio(oc, audio_st);

    /* Free the streams. */
    {
        int i;
        for (i = 0; i < oc->nb_streams; i++) {
            av_freep(&oc->streams[i]->codec);
            av_freep(&oc->streams[i]);
        }
    }

    if (!(fmt->flags & AVFMT_NOFILE)) {
        /* Close the output file. */
        avio_close(oc->pb);
   }
    else {
        av_free(oc->pb);
        free(e->io_buffer);
        e->io_buffer = 0;
    }
    /* free the stream */
    av_free(oc);

    return;
}

void writeVideoFrame(MyEncoder *e, AVFrame *frame)
{
    if(frame)
    {
        write_video_frame2(e->oc, e->video_st, frame);
    }
}

void writeAudio(MyEncoder *e, AVFrame *frame)
{
    if(frame)
    {
        write_audio_frame2(e->oc, e->audio_st, frame);
    }
}

AVFrame *allocFrame(int width, int height)
{
    AVFrame *frame = av_frame_alloc();
    memset(frame->linesize, 0, sizeof(frame->linesize));
    memset(frame->data, 0, sizeof(frame->data));
    frame->linesize[0] = width;
    frame->linesize[1] = width;
    frame->linesize[2] = width;
    frame->data[0] = (uint8_t*)malloc( width * height + 1024);
    frame->data[1] = (uint8_t*)malloc( width * height + 1024);
    frame->data[2] = (uint8_t*)malloc( width * height + 1024);
    frame->width = width;
    frame->height = height;
    return frame;
}

void freeFrame(AVFrame *frame)
{
    if(frame)
    {
        int i;
        for(i = 0; i < AV_NUM_DATA_POINTERS; ++i)
        {
            if(frame->data)
            {
                free(frame->data[i]);
                frame->data[i] = NULL;
            }
        }
        av_frame_free(&frame);
    }
}
