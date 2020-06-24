#include "X264Encoder.h"

#include <x264.h>
#include <QString>

#include "ffwrite.h"

X264Encoder::X264Encoder(AVFormatContext *oc, AVStream *video_st)
    : m_encoder(nullptr)
    , m_pic_in(nullptr)
    , m_width(0)
    , m_height(0)
    , m_h264_fp(nullptr)
    , m_nals(nullptr)
    , m_i_nals(0)
    , m_out_dir("/Users/oosman/Documents")
//    , m_fmt(nullptr)
    , m_oc(oc)
//    , m_audio_codec(nullptr)
//    , m_video_codec(nullptr)
    , m_video_st(video_st)
{
//    avformat_alloc_output_context2(&m_oc, NULL, NULL,
//                                   "C:\\Users\\oosman\\Documents\\foo.mp4");
//    m_fmt = m_oc->oformat;

//    m_video_st = avformat_new_stream(m_oc, NULL);
//    m_video_st->id = m_oc->nb_streams - 1;

//    c = avcodec_alloc_context3(m_video_codec);

//    for(int i = 0; i < 3; ++i)
//    {
//        m_plane[i] = nullptr;
//    }
}

X264Encoder::~X264Encoder()
{
    finish();
}

void X264Encoder::finish()
{
//    if(m_pic_in)
//    {
//        x264_picture_clean(m_pic_in);
//    }
    if(m_h264_fp)
    {
        fclose(m_h264_fp);
        m_h264_fp = nullptr;
    }
//    for(int i = 0; i < 3; ++i)
//    {
//        if(m_plane[i])
//        {
//            delete [] m_plane[i];
//            m_plane[i] = nullptr;
//        }
//    }

    delete m_pic_in;
    m_pic_in = nullptr;
}

bool X264Encoder::init(int width, int height, AVRational &timebase, int fps)
{
    x264_param_t param;
    if( x264_param_default_preset( &param, "medium", NULL ) < 0 )
    { return false; }

    /* Configure non-default params */
    param.i_bitdepth = 8;
    param.i_csp = X264_CSP_I420;
    param.i_width  = m_width = width;
    param.i_height = m_height = height;
    param.b_vfr_input = 0;
    param.b_repeat_headers = 1;
    param.b_annexb = 1;
    if(0)
    {
        param.b_vfr_input = 0;
        param.i_fps_num = 4;
        param.i_fps_den = 1;
    }
    else
    {
        param.b_vfr_input = 1;
        param.i_timebase_num = timebase.den * 3.2;//why this magic number? I don't know!
        param.i_timebase_den = timebase.num;
    }

    /* Apply profile restrictions. */
    if( x264_param_apply_profile(&param, "high" ) < 0 )
    { return false; }

    finish();
    m_pic_in = new x264_picture_t;
    x264_picture_init(m_pic_in);
    m_pic_in->img.i_csp = X264_CSP_I420;
    m_pic_in->img.i_plane = 3;

    m_encoder = x264_encoder_open(&param);//todo crash problem

    m_h264_fp = fopen(QString("%1/frame%2x%3.h264").
                      arg(m_out_dir).
                      arg(m_width).
                      arg(m_height).
                      toUtf8().data(),
                     "wb");
//    for(int i = 0; i < 3; ++i)
//    {
//        m_plane[i] = new uint8_t [m_width * m_height];
//    }
    return true;
}

void X264Encoder::writeH264Frame(uint8_t *data, int size)
{
    fwrite(data, size, 1, m_h264_fp);
//    {
//        AVPacket pkt;
//        av_init_packet(&pkt);
//        pkt.data = data;
//        pkt.size = size;

//        ::writePacket(m_oc, m_video_st, &pkt);
//    }
}

void X264Encoder::encode(uint8_t *plane[4], int64_t pts)
{
    //TODO: Save yuv frames using these m_plane(s),
    //Then use x264_encoder_encode to convert yuv to h264
    //Then save m_nals->p_payload, frame_size bytes to h264 file
    //Then run ffmpeg.exe -i /c/Users/oosman/Documents/frame1366x768.h264 -c:v copy -f mp4  /c/Users/oosman/Documents/frame1366x768.mp4
//    {
//        int luma_size = m_width * m_height;
//        int chroma_size = luma_size / 4;
//        memcpy(m_plane[0], plane[0], luma_size);
//        memcpy(m_plane[1], plane[1], luma_size);
//        memcpy(m_plane[2], plane[2], luma_size);
//    }
    m_pic_in->img.plane[0] = plane[0];
    m_pic_in->img.i_stride[0] = m_width;
    m_pic_in->img.plane[1] = plane[1];
    m_pic_in->img.i_stride[1] = m_width;
    m_pic_in->img.plane[2] = plane[2];
    m_pic_in->img.i_stride[2] = m_width;

    x264_picture_t pic_out;
    m_pic_in->i_pts = pts;
    int frame_size = x264_encoder_encode(m_encoder, &m_nals, &m_i_nals, m_pic_in, &pic_out);

    if(frame_size <= 0)
    {
        return;
    }

    //TODO: USE LIBAVCODEC GPL TO CONVERT TO MP4
    writeH264Frame(m_nals->p_payload, frame_size);
}

bool X264Encoder::flush()
{
    while(x264_encoder_delayed_frames(m_encoder))
    {
        x264_picture_t pic_out;
        int frame_size = x264_encoder_encode(m_encoder, &m_nals, &m_i_nals, NULL, &pic_out );
        if( frame_size < 0 )
        { return false; }
        else if( frame_size > 0 )
        {
            writeH264Frame(m_nals->p_payload, frame_size);
        }
    }

    x264_encoder_close(m_encoder);

    finish();
    return true;
}
