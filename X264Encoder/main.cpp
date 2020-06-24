#include <QCoreApplication>
#include <QDebug>
#include <QProcess>

#include "X264Encoder.h"
#include "libavutil/rational.h"
#include "FFMpegRunner.h"

int main_convert_yuv_to_h264(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    X264Encoder x264(nullptr, nullptr);

    FILE* yuv_fp = fopen("/Users/oosman/Documents/frames1366x768.yuv", "rb");
    if(!yuv_fp)
    {
        return 0;
    }

    char header[] = "OsLeYuvFormat";
    fread(header, sizeof(header), 1, yuv_fp);
    int version = 1;
    fread(&version, sizeof(version), 1, yuv_fp);
    int width = 0;
    fread(&width, sizeof(width), 1, yuv_fp);
    int height = 0;
    fread(&height, sizeof(height), 1, yuv_fp);
    AVRational time_base;
    fread(&time_base, sizeof(AVRational), 1, yuv_fp);
    int fps = 0;
    fread(&fps, sizeof(fps), 1, yuv_fp);

    x264.init(width, height, time_base, fps);

    int luma_size = width * height;
    int chroma_size = luma_size / 4;

    uint8_t* plane[3];
    for(int i = 0; i < 3; ++i)
    {
        plane[i] = new uint8_t [width * height];
    }

    int frame;
    for(frame = 0;; ++frame)
    {
        size_t bytes = 0;
        bytes = fread(plane[0], luma_size, 1, yuv_fp);
        if(bytes != 1) { break; }
        bytes = fread(plane[1], luma_size, 1, yuv_fp);
        if(bytes != 1) { break; }
        bytes = fread(plane[2], luma_size, 1, yuv_fp);
        if(bytes != 1) { break; }

        int64_t pts = 0;
        bytes = fread(&pts, sizeof(pts), 1, yuv_fp);
        if(bytes != 1) { break; }

        int64_t diff = 0;
        bytes = fread(&diff, sizeof(diff), 1, yuv_fp);
        if(bytes != 1) { break; }
        qDebug() << "time diff" << diff << "ms";

        x264.encode(plane, pts);
    }
    qDebug() << "num frames" << frame;
    for(int i = 0; i < 3; ++i)
    {
        delete [] plane[i];
        plane[i] = nullptr;
    }
    x264.flush();

    fclose(yuv_fp);

    return 0;//a.exec();
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    FFMpegRunner ffmpeg;
    ffmpeg.convertToH264(argv[1]);
    a.exec();
}
