/*
 * copyright (c) 2001 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "ffplay3.h"
#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extern void mylog(const char *fmt, ...);

//const int program_birth_year = 2016;
//const char program_name[] = "test";

static int decode_interrupt_cb(void *ctx)
{
    return 0;
}

//void show_help_default(const char *opt, const char *arg)
//{
//}

void decoder_register()
{
    avdevice_register_all();
    avfilter_register_all();
    av_register_all();
    avformat_network_init();
    init_opts();
}

static void decoder_init(Decoder *d)
{
    int i;
    d->packet_pending = 0;
    d->start_pts = d->next_pts = AV_NOPTS_VALUE;
    d->ic = avformat_alloc_context();
    av_init_packet(&d->pkt_temp);
    av_init_packet(&d->flush_pkt);
    for(i=0; i<AVMEDIA_TYPE_NB; ++i)
    {
        d->stream[i] = NULL;
        d->avctx[i] = NULL;
    }
    d->format_opts1 = NULL;
    d->codec_opts1 = NULL;

    //osm
    d->buffer_ = NULL;
    d->buffer_size_ = 8*1024;
    d->buffer_ =( unsigned char *)av_malloc(d->buffer_size_);
    d->ic->pb = avio_alloc_context(d->buffer_,
                                   d->buffer_size_,
                                   0,
                                   d->user_data,
                                   d->readfn_,
                                   0,
                                   d->seekfn_);
    return;//osm

}

void decoder_close(Decoder *d)
{
    int i;
    for(i=0; i<AVMEDIA_TYPE_NB; ++i)
    {
        if(d->avctx[i])
        {
            avcodec_close(d->avctx[i]);
        }
        d->stream[i] = NULL;
        d->avctx[i] = NULL;
    }
    avformat_close_input(&d->ic);
//    av_packet_unref(&d->pkt_temp);
    av_packet_unref(&d->flush_pkt);
    av_freep(&d->format_opts1);
    av_freep(&d->codec_opts1);
    d->format_opts1 = NULL;
    d->codec_opts1 = NULL;
//    if(d->buffer_) { av_free(d->buffer_); }
//    d->buffer_ = NULL;
    d->buffer_size_ = 0;
    if(d->fp_) {  close(d->fp_); }
    d->fp_ = NULL;
    if(d->filename) { free(d->filename); }
    d->filename = NULL;
}

static int durationMilliseconds(Decoder *d)
{
    return d->ic->duration * 1000 / AV_TIME_BASE;
}

static void readinfo(Decoder *d)
{
    AVStream *data_st = d->ic->streams[d->st_index[AVMEDIA_TYPE_DATA]];
    AVStream *video_st = d->st_index[AVMEDIA_TYPE_VIDEO] > -1 ?
                d->ic->streams[d->st_index[AVMEDIA_TYPE_VIDEO]] : 0;
    AVStream *audio_st = d->st_index[AVMEDIA_TYPE_AUDIO] > -1 ?
                d->ic->streams[d->st_index[AVMEDIA_TYPE_AUDIO]] : 0;

    int stream_index = d->st_index[AVMEDIA_TYPE_VIDEO];
    d->info.width = d->avctx[stream_index]->width;
    if(d->info.width == 0)
    {
        d->info.width =
                d->avctx[stream_index]->width =
                video_st->codec->width;
    }
    if(d->info.width == 0)
    {
        d->info.width =
                d->avctx[stream_index]->width =
                video_st->codec->coded_width;
    }
    d->info.height = d->avctx[stream_index]->height;
    if(d->info.height == 0)
    {
        d->info.height =
                d->avctx[stream_index]->height =
                video_st->codec->height;
    }
    if(d->info.height == 0)
    {
        d->info.height =
                d->avctx[stream_index]->height =
                video_st->codec->coded_height;
    }

    d->info.duration = d->ic->duration;
    d->info.num_frames = video_st->nb_frames;

    d->info.frame_rate = (int)round(av_q2d(video_st->avg_frame_rate));
    if(d->info.frame_rate <= 0)
    {
        d->info.frame_rate = (int)round(av_q2d(data_st->avg_frame_rate));
    }
    if(d->info.frame_rate <= 0)
    {
        d->info.frame_rate = (int)round(av_q2d(video_st->r_frame_rate));
    }
    mylog("avg_frame_rate %i\n", d->info.frame_rate);
//    AVRational r =
//            av_guess_frame_rate(
//                d->avctx[d->st_index[AVMEDIA_TYPE_VIDEO]],
//            d->ic->streams[d->st_index[AVMEDIA_TYPE_VIDEO]], 0);
//            av_stream_get_r_frame_rate(d->ic->streams[d->st_index[AVMEDIA_TYPE_VIDEO]]);
//            d->ic->streams[d->st_index[AVMEDIA_TYPE_VIDEO]]->r_frame_rate;
//    d->ic->streams[d->st_index[AVMEDIA_TYPE_VIDEO]]->time_base
//    d->info.frame_rate = r.num/r.den;
//    https://stackoverflow.com/questions/12234949/ffmpeg-time-unit-explanation-and-av-seek-frame-method
//        int ticks_per_frame = d->avctx[d->st_index[AVMEDIA_TYPE_VIDEO]]->ticks_per_frame;
//        AVRational r = d->ic->streams[d->st_index[AVMEDIA_TYPE_VIDEO]]->time_base;
//        d->info.frame_rate = r.den/r.num/ticks_per_frame;
}

static void printDuration(Decoder *d)
{
    int ms = durationMilliseconds(d);
    if(ms > 60*1000) {
        printf("duration %i minutes, %.02f seconds\n", ms/1000/60, (ms - (ms/1000/60) * (1000*60))/1000.0);
    }
    else if(ms > 1000) {
        printf("duration %.02f seconds\n", ms/1000.0);
    }
    else  {
        printf("duration %i ms\n", ms);
    }
}

static int decode_frame(Decoder *d, AVPacket *pkt, AVFrame* frame) {
    int got_frame = 0;
    int decoder_reorder_pts = -1;
//    int finished;

//    do {
        int ret = -1;

//        if (d->queue->abort_request)
//            return -1;

        if (!d->packet_pending/* || d->queue->serial != d->pkt_serial*/) {
//            AVPacket pkt;
            do {
//                if (d->queue->nb_packets == 0)
//                    SDL_CondSignal(d->empty_queue_cond);
//                if (packet_queue_get(d->queue, &pkt, 1, &d->pkt_serial) < 0)
//                    return -1;
                if (pkt->data == d->flush_pkt.data) {
                    if(pkt->stream_index == d->st_index[AVMEDIA_TYPE_VIDEO]) {
                        avcodec_flush_buffers(d->avctx[pkt->stream_index]);
                    }
                    else if(pkt->stream_index == d->st_index[AVMEDIA_TYPE_AUDIO]) {
                        avcodec_flush_buffers(d->avctx[pkt->stream_index]);
                    }
//                    finished = 0;
                    d->next_pts = d->start_pts;
                    d->next_pts_tb = d->start_pts_tb;
                    break;
                }
            } while (pkt->data == d->flush_pkt.data/* || d->queue->serial != d->pkt_serial*/);
            d->pkt_temp = *pkt;
            d->packet_pending = 1;
        }

//        switch (d->avctx->codec_type) {
        if((pkt->stream_index == d->st_index[AVMEDIA_TYPE_VIDEO] ||
            pkt->stream_index == d->st_index[AVMEDIA_TYPE_DATA]) &&
                d->avctx[pkt->stream_index])
        {
//            TODO: use this api which replaces the deprecated video2 api
//            ret = avcodec_receive_frame(d->avctx[pkt->stream_index], frame);
//            if(ret == 0)
            ret = avcodec_decode_video2(d->avctx[pkt->stream_index], frame,
                    &got_frame, &d->pkt_temp);
            if (got_frame)
            {
                if (decoder_reorder_pts == -1) {
                    frame->pts = av_frame_get_best_effort_timestamp(frame);
                } else if (decoder_reorder_pts) {
                    frame->pts = frame->pkt_pts;
                } else {
                    frame->pts = frame->pkt_dts;
                }
            }
        }
        else if(pkt->stream_index == d->st_index[AVMEDIA_TYPE_AUDIO] &&
                d->avctx[pkt->stream_index])
        {
                ret = avcodec_decode_audio4(d->avctx[pkt->stream_index], frame, &got_frame, &d->pkt_temp);
                if (got_frame) {
                    AVRational tb = (AVRational){1, frame->sample_rate};
                    if (frame->pts != AV_NOPTS_VALUE)
                        frame->pts = av_rescale_q(frame->pts, d->avctx[pkt->stream_index]->time_base, tb);
                    else if (frame->pkt_pts != AV_NOPTS_VALUE)
                        frame->pts = av_rescale_q(frame->pkt_pts, av_codec_get_pkt_timebase(d->avctx[pkt->stream_index]), tb);
                    else if (d->next_pts != AV_NOPTS_VALUE)
                        frame->pts = av_rescale_q(d->next_pts, d->next_pts_tb, tb);
                    if (frame->pts != AV_NOPTS_VALUE) {
                        d->next_pts = frame->pts + frame->nb_samples;
                        d->next_pts_tb = tb;
                    }
                }
        }
        else if(pkt->stream_index == d->st_index[AVMEDIA_TYPE_SUBTITLE])
        {
            mylog("#### got subtitle");
//                ret = avcodec_decode_subtitle2(d->avctx[?], sub, &got_frame, &d->pkt_temp);
        }
        else if(pkt->stream_index == d->st_index[AVMEDIA_TYPE_ATTACHMENT])
        {
            mylog("#### got attachment");
//                ret = avcodec_decode_subtitle2(d->avctx[?], sub, &got_frame, &d->pkt_temp);
        }
//        else if(pkt->stream_index == d->st_index[AVMEDIA_TYPE_DATA])
//        {
//            mylog("#### got data");
////                ret = avcodec_decode_data(d->avctx[?], sub, &got_frame, &d->pkt_temp);
//        }

        if (ret < 0) {
            d->packet_pending = 0;
        } else {
            d->pkt_temp.dts =
            d->pkt_temp.pts = AV_NOPTS_VALUE;
            if (d->pkt_temp.data) {
//                if (d->avctx[pkt->stream_index] &&
//                        d->avctx[pkt->stream_index]->codec_type != AVMEDIA_TYPE_AUDIO) {
//                    ret = d->pkt_temp.size;
//                }
                d->pkt_temp.data += ret;
                d->pkt_temp.size -= ret;
                if (d->pkt_temp.size <= 0)
                    d->packet_pending = 0;
            } else {
                if (!got_frame) {
                    d->packet_pending = 0;
//                    d->finished = d->pkt_serial;
                }
            }
        }
//    } while (!got_frame && !finished);

    return got_frame;
}

static int packet_queue_put(Decoder *d, enum AVMediaType media_type, AVPacket *pkt)
{
    int ret = 0;
    AVFrame* frame = av_frame_alloc();
    if(decode_frame(d, pkt, frame) != 1 || !d->gotFrameCb)
    {
        av_frame_free(&frame);
    }
    else
    {
        if(0 && media_type == AVMEDIA_TYPE_AUDIO)
        {
            int audio_stream_index = d->st_index[AVMEDIA_TYPE_AUDIO];
            FILE *fp = fopen("a.pcm", "a+b");
            int data_size = av_samples_get_buffer_size(NULL, frame->channels,
                                                       frame->nb_samples,
                                                       d->avctx[audio_stream_index]->sample_fmt, 1);

            int sample_rate = d->avctx[audio_stream_index]->sample_rate;
            int num_channels = d->avctx[audio_stream_index]->channels;
            int i;
            for(i = 0; i < 1/*frame->channels*/; i++)
            {
                fwrite(frame->extended_data[i], 1, frame->linesize[0], fp);
            }
            fclose(fp);

            mylog("%i,%i,%i,%i,%i,%i",
                  num_channels,
                  frame->format,
                  frame->sample_rate,
                  frame->nb_samples,
                  frame->linesize[0],
                  data_size);
        }
//        if(media_type == AVMEDIA_TYPE_VIDEO ||
//                media_type == AVMEDIA_TYPE_AUDIO)
        {
            ret = (*d->gotFrameCb)(d->user_data, media_type, frame, pkt->dts, pkt->size) == 1 ? 0:-1;
        }
    }
    av_packet_unref(pkt);
    return ret;
}

static int packet_queue_put_nullpacket(Decoder *d, enum AVMediaType media_type, int stream_index)
{
    AVPacket pkt1, *pkt = &pkt1;
    av_init_packet(pkt);
    pkt->data = NULL;
    pkt->size = 0;
    pkt->stream_index = stream_index;
    return packet_queue_put(d, media_type, pkt);
}

/* open a given stream. Return 0 if OK */
static int stream_component_open(Decoder *d, enum AVMediaType media_type, AVDictionary *codec_opts)
{
    AVCodec *codec;
    AVDictionary *opts;
    AVDictionaryEntry *t = NULL;
    int ret = 0;
    int stream_index = d->st_index[media_type];
    int stream_lowres = 0;

    if (stream_index < 0 || stream_index >= d->ic->nb_streams)
        return -1;
    d->avctx[stream_index] = d->ic->streams[stream_index]->codec;

    codec = avcodec_find_decoder(d->avctx[stream_index]->codec_id);

    if (!codec) {
        av_log(NULL, AV_LOG_WARNING,
                                      "No codec could be found with id %d\n", d->avctx[stream_index]->codec_id);
        return -1;
    }

    d->avctx[stream_index]->codec_id = codec->id;
    if(stream_lowres > av_codec_get_max_lowres(codec)){
        av_log(d->avctx[stream_index], AV_LOG_WARNING, "The maximum value for lowres supported by the decoder is %d\n",
                av_codec_get_max_lowres(codec));
        stream_lowres = av_codec_get_max_lowres(codec);
    }
    av_codec_set_lowres(d->avctx[stream_index], stream_lowres);

#if FF_API_EMU_EDGE
    if(stream_lowres) d->avctx[stream_index]->flags |= CODEC_FLAG_EMU_EDGE;
#endif
//    if (fast)
//        d->avctx[stream_index]->flags2 |= AV_CODEC_FLAG2_FAST;
#if FF_API_EMU_EDGE
    if(codec->capabilities & AV_CODEC_CAP_DR1)
        d->avctx[stream_index]->flags |= CODEC_FLAG_EMU_EDGE;
#endif

    opts = filter_codec_opts(codec_opts, d->avctx[stream_index]->codec_id, d->ic, d->ic->streams[stream_index], codec);
    if (!av_dict_get(opts, "threads", NULL, 0))
        av_dict_set(&opts, "threads", "auto", 0);
    if (stream_lowres)
        av_dict_set_int(&opts, "lowres", stream_lowres, 0);
    if (d->avctx[stream_index]->codec_type == AVMEDIA_TYPE_VIDEO || d->avctx[stream_index]->codec_type == AVMEDIA_TYPE_AUDIO)
        av_dict_set(&opts, "refcounted_frames", "1", 0);
    if ((ret = avcodec_open2(d->avctx[stream_index], codec, &opts)) < 0) {
        goto fail;
    }
    if ((t = av_dict_get(opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
        av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
        ret =  AVERROR_OPTION_NOT_FOUND;
        goto fail;
    }

    d->ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;
    d->stream[media_type] = d->ic->streams[stream_index];

fail:
    av_dict_free(&opts);

    return ret;
}

static void freeDictionary(int orig_nb_streams, AVDictionary **opts)
{
    int i;
    for (i = 0; i < orig_nb_streams; i++)
        av_dict_free(&opts[i]);
    av_freep(&opts);
}

/* this thread gets the stream from the disk or the network */
int decoder_open(Decoder *d)
{
    AVInputFormat *iformat = NULL;
    AVPacket flush_pkt;
    int err, i, ret = 0;
    AVDictionaryEntry *t;
    AVDictionary **opts;
    int orig_nb_streams;
    int scan_all_pmts_set = 0;
    int genpts = 0;
//    int seek_by_bytes = -1;
    int max_frame_duration;
    int64_t start_time = AV_NOPTS_VALUE;

    flush_pkt.data = (uint8_t *)&flush_pkt;

    decoder_init(d);
    if (!d->ic) {
        av_log(NULL, AV_LOG_FATAL, "Could not allocate context.\n");
        ret = AVERROR(ENOMEM);
        goto fail;
    }

    d->ic->interrupt_callback.callback = decode_interrupt_cb;
    d->ic->interrupt_callback.opaque = NULL;
    if (!av_dict_get(d->format_opts1, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE)) {
        av_dict_set(&d->format_opts1, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
        scan_all_pmts_set = 1;
    }

    err = avformat_open_input(&d->ic, d->filename, iformat, &d->format_opts1);
    if (err < 0) {
        print_error(d->filename, err);
        av_freep(&d->format_opts1);
        ret = -1;
        goto fail;
    }
    if (scan_all_pmts_set) {
        av_dict_set(&d->format_opts1, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE);
    }

    if ((t = av_dict_get(d->format_opts1, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
        av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
        ret = AVERROR_OPTION_NOT_FOUND;
        av_freep(&d->format_opts1);
        goto fail;
    }

    if (genpts) {
        d->ic->flags |= AVFMT_FLAG_GENPTS;
    }

//    av_format_inject_global_side_data(ic);

    opts = setup_find_stream_info_opts(d->ic, d->codec_opts1);
    orig_nb_streams = d->ic->nb_streams;

    err = avformat_find_stream_info(d->ic, opts);

    freeDictionary(orig_nb_streams, opts);

    if (err < 0) {
        av_log(NULL, AV_LOG_WARNING,
               "%s: could not find codec parameters\n", d->filename);
        ret = -1;
        goto fail;
    }

    if (d->ic->pb)
        d->ic->pb->eof_reached = 0; // FIXME hack, ffplay maybe should not use avio_feof() to test for the end

//    if (seek_by_bytes < 0)
//        seek_by_bytes = !!(d->ic->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", d->ic->iformat->name);

    max_frame_duration = (d->ic->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;

    /* if seeking requested, we execute it */
    if (start_time != AV_NOPTS_VALUE) {
        int64_t timestamp;

        timestamp = start_time;
        /* add the stream start time */
        if (d->ic->start_time != AV_NOPTS_VALUE)
            timestamp += d->ic->start_time;
        ret = avformat_seek_file(d->ic, -1, INT64_MIN, timestamp, INT64_MAX, 0);
        if (ret < 0) {
            av_log(NULL, AV_LOG_WARNING, "%s: could not seek to position %0.3f\n",
                    d->filename, (double)timestamp / AV_TIME_BASE);
        }
    }

    av_dump_format(d->ic, 0, d->filename, 0);

    d->st_index[AVMEDIA_TYPE_VIDEO] =
            d->st_index[AVMEDIA_TYPE_AUDIO] =
            d->st_index[AVMEDIA_TYPE_SUBTITLE] = -1;

    for (i = 0; i < d->ic->nb_streams; i++) {
        AVStream *st = d->ic->streams[i];
        enum AVMediaType type = st->codec->codec_type;
        st->discard = AVDISCARD_ALL;
        d->st_index[type] = i;
    }

        d->st_index[AVMEDIA_TYPE_VIDEO] =
            av_find_best_stream(d->ic, AVMEDIA_TYPE_VIDEO,
                                d->st_index[AVMEDIA_TYPE_VIDEO], -1, NULL, 0);
        d->st_index[AVMEDIA_TYPE_AUDIO] =
            av_find_best_stream(d->ic, AVMEDIA_TYPE_AUDIO,
                                d->st_index[AVMEDIA_TYPE_AUDIO],
                                -1,
                                NULL, 0);
        d->st_index[AVMEDIA_TYPE_SUBTITLE] =
            av_find_best_stream(d->ic, AVMEDIA_TYPE_SUBTITLE,
                                d->st_index[AVMEDIA_TYPE_SUBTITLE],
                                (d->st_index[AVMEDIA_TYPE_AUDIO] >= 0 ?
                                 d->st_index[AVMEDIA_TYPE_AUDIO] :
                                 d->st_index[AVMEDIA_TYPE_VIDEO]),
                                NULL, 0);

//    if (d->st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
//        AVStream *st = d->ic->streams[d->st_index[AVMEDIA_TYPE_VIDEO]];
//        AVRational sar = av_guess_sample_aspect_ratio(d->ic, st, NULL);
//        if (st->codec->width){
//            set_default_window_size(st->codec->width, st->codec->height, sar);
//        }
//    }

        ret = -1;
    /* open the streams */
    if (d->st_index[AVMEDIA_TYPE_AUDIO] >= 0) {
        ret = stream_component_open(d, AVMEDIA_TYPE_AUDIO, d->codec_opts1);
    }

    if (d->st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
        ret = stream_component_open(d, AVMEDIA_TYPE_VIDEO, d->codec_opts1);
    }

    if (d->st_index[AVMEDIA_TYPE_SUBTITLE] >= 0) {
        stream_component_open(d, AVMEDIA_TYPE_SUBTITLE, d->codec_opts1);
    }

    if (d->st_index[AVMEDIA_TYPE_DATA] >= 0) {
        stream_component_open(d, AVMEDIA_TYPE_DATA, d->codec_opts1);
    }

    if (d->st_index[AVMEDIA_TYPE_ATTACHMENT] >= 0) {
        stream_component_open(d, AVMEDIA_TYPE_ATTACHMENT, d->codec_opts1);
    }

    readinfo(d);
    printDuration(d);

    d->ic->pb->read_packet = d->readfn_;
    d->ic->pb->seek = d->seekfn_;
fail:
    return ret;
}

static int64_t time_from_start(Decoder *d, AVPacket *pkt)
{
    int64_t pkt_ts = ( pkt->pts == AV_NOPTS_VALUE ) ? pkt->dts : pkt->pts;
    int64_t stream_start_time = d->ic->streams[pkt->stream_index]->start_time;
    stream_start_time = (stream_start_time != AV_NOPTS_VALUE) ? stream_start_time : 0;
    stream_start_time *= av_q2d(d->ic->streams[pkt->stream_index]->time_base);
    int64_t diff = pkt_ts - stream_start_time;

    return diff;
}

static int in_range(Decoder *d, AVPacket *pkt, int64_t seek_ms)
{
    int pkt_in_play_range = 1;

//    pkt_in_play_range = d->info.duration == AV_NOPTS_VALUE ||
//            (pkt_ts - (stream_start_time != AV_NOPTS_VALUE ? stream_start_time : 0)) *
//            av_q2d(d->ic->streams[pkt->stream_index]->time_base) -
//            (double)(start_time != AV_NOPTS_VALUE ? start_time : 0) / 1000000
//            <= ((double)d->info.duration / 1000000);
    /*if ( (pkt_ts - (stream_start_time != AV_NOPTS_VALUE ? stream_start_time : 0)) *
              av_q2d(d->ic->streams[pkt->stream_index]->time_base) -
              (double)(seek_time != AV_NOPTS_VALUE ? seek_time : 0) / 1000000
              <= ((double)d->info.duration / 1000000) )*/
    if ( seek_ms > -1 &&
         d->stream[pkt->stream_index] &&
         d->stream[pkt->stream_index]->time_base.den > 0)
    {
        int64_t diff = time_from_start(d, pkt);

        int64_t seek_time = (seek_ms / 1000) / av_q2d (d->stream[pkt->stream_index]->time_base);
        seek_time = (seek_time != AV_NOPTS_VALUE) ? seek_time : 0;

        pkt_in_play_range = (diff >= seek_time);

    }
    return pkt_in_play_range;
}

static void seek_start(Decoder *d)
{
    int stream_index = d->st_index[AVMEDIA_TYPE_VIDEO];
    if(d->avctx[stream_index]->codec_id == AV_CODEC_ID_MPEG4) {
//        decoder_close(d);
//        decoder_open(d);
    }
    else {
        av_seek_frame(d->ic, -1, 0, AVSEEK_FLAG_ANY);
    }
}

void decoder_play(Decoder *d, int seek_ms)
{
    int ret = 0;
    int eof = 0;
    int brk = 0;
    AVPacket pkt1, *pkt = &pkt1;
    int pkt_in_play_range = 0;

    av_init_packet(pkt);
    pkt->data = NULL;
    pkt->size = 0;

    seek_start(d);

    while(!eof && !brk)
    {
        ret = av_read_frame(d->ic, pkt);//TODO: for fantastic beasts movie, it keeps returning pkts for audio stream only
        if (ret < 0) {
            if ((ret == AVERROR_EOF || avio_feof(d->ic->pb)) && !eof) {
                if (d->st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
                    mylog("packet_queue_put_nullpacket video");
                    brk = packet_queue_put_nullpacket(d, AVMEDIA_TYPE_VIDEO, d->st_index[AVMEDIA_TYPE_VIDEO]);
                }
                else if (d->st_index[AVMEDIA_TYPE_AUDIO] >= 0) {
                    brk = packet_queue_put_nullpacket(d, AVMEDIA_TYPE_AUDIO, d->st_index[AVMEDIA_TYPE_AUDIO]);
                }
                else if (d->st_index[AVMEDIA_TYPE_SUBTITLE] >= 0) {
                    brk = packet_queue_put_nullpacket(d, AVMEDIA_TYPE_SUBTITLE, d->st_index[AVMEDIA_TYPE_SUBTITLE]);
                }
                else if (d->st_index[AVMEDIA_TYPE_DATA] >= 0) {
                    brk = packet_queue_put_nullpacket(d, AVMEDIA_TYPE_DATA, d->st_index[AVMEDIA_TYPE_DATA]);
                }
                else if (d->st_index[AVMEDIA_TYPE_ATTACHMENT] >= 0) {
                    brk = packet_queue_put_nullpacket(d, AVMEDIA_TYPE_ATTACHMENT, d->st_index[AVMEDIA_TYPE_ATTACHMENT]);
                }
                eof = 1;
            }
            if (d->ic->pb && d->ic->pb->error) {
                brk = 1;
            }
            continue;
        } else {
            eof = 0;
        }
        pkt_in_play_range = in_range(d, pkt, seek_ms);

        if(pkt_in_play_range) {
            if (pkt->stream_index == d->st_index[AVMEDIA_TYPE_AUDIO]) {
                brk = packet_queue_put(d, AVMEDIA_TYPE_AUDIO, pkt);
            } else if (pkt->stream_index == d->st_index[AVMEDIA_TYPE_VIDEO]
                       /*&& !(d->stream[AVMEDIA_TYPE_VIDEO]->disposition & AV_DISPOSITION_ATTACHED_PIC)*/) {
                brk = packet_queue_put(d, AVMEDIA_TYPE_VIDEO, pkt);
            } else if (pkt->stream_index == d->st_index[AVMEDIA_TYPE_SUBTITLE]) {
                brk = packet_queue_put(d, AVMEDIA_TYPE_SUBTITLE, pkt);
            } else if (pkt->stream_index == d->st_index[AVMEDIA_TYPE_DATA]) {
                brk = packet_queue_put(d, AVMEDIA_TYPE_DATA, pkt);
            } else if (pkt->stream_index == d->st_index[AVMEDIA_TYPE_ATTACHMENT]) {
                brk = packet_queue_put(d, AVMEDIA_TYPE_ATTACHMENT, pkt);
            }
        }
        av_packet_unref(pkt);

    }
}

