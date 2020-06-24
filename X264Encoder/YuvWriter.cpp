#include "YuvWriter.h"

#include <QString>
#include <time.h>

#include "libavutil/rational.h"
#include "ffwrite.h"

YuvWriter::YuvWriter()
    : m_yuv_fp(nullptr)
{

}

/*
  Call open from VideoFrameProducer::startVideoRecording like this
    m_yuv_writer = new YuvWriter;
    m_yuv_writer->open(width, height,
                       timebase(m_encoder.video_st),
                       m_encoder.frames_per_second);
*/
bool YuvWriter::open(int width, int height, AVRational &&time_base, int fps)
{
    if(m_yuv_fp)
    {
        return true;
    }

    m_yuv_fp = fopen(QString("%1/frames%2x%3.yuv").
                     arg("/Users/oosman/Documents").
                     arg(width).
                     arg(height).
                     toUtf8().data(),
                     "wb");
    if(m_yuv_fp)
    {
        char header[] = "OsLeYuvFormat";
        fwrite(header, sizeof(header), 1, m_yuv_fp);
        int version = 1;
        fwrite(&version, sizeof(version), 1, m_yuv_fp);
        fwrite(&width, sizeof(width), 1, m_yuv_fp);
        fwrite(&height, sizeof(height), 1, m_yuv_fp);
        fwrite(&time_base, sizeof(AVRational), 1, m_yuv_fp);
        fwrite(&fps, sizeof(fps), 1, m_yuv_fp);

        m_time_stamp_of_prev_frame = 0;
        return true;
    }
    return false;
}

/*
    call close from VideoFrameProducer::stopVideoRecording() like this
    m_yuv_writer->close();
    delete m_yuv_writer;
    m_yuv_writer = nullptr;
*/
void YuvWriter::close()
{
    if(m_yuv_fp)
    {
        fclose(m_yuv_fp);
        m_yuv_fp = nullptr;
    }
}

/*
    call write from VideoFrameProducer::writeFrame like this
    write(frame->data, img.width(), img.height(), frame->pts);
    just before ::writeVideoFrame
*/
void YuvWriter::write(uint8_t *plane[4], int width, int height, int64_t pts)
{
    if(!m_yuv_fp)
    {
        return;
    }

    int luma_size = width * height;
    int chroma_size = luma_size / 4;

    fwrite(plane[0], luma_size, 1, m_yuv_fp);
    fwrite(plane[1], luma_size, 1, m_yuv_fp);
    fwrite(plane[2], luma_size, 1, m_yuv_fp);

    fwrite(&pts, sizeof(pts), 1, m_yuv_fp);

    int64_t now = getTickCount();
    int64_t diff = (m_time_stamp_of_prev_frame == 0) ?
                0 : (now - m_time_stamp_of_prev_frame);
    m_time_stamp_of_prev_frame = now;

    fwrite(&diff, sizeof(diff), 1, m_yuv_fp);
}


