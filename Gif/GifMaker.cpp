#include "GifMaker.h"

#include <QThread>
#include <gif_lib.h>
#include <QDebug>
#include <QElapsedTimer>

#include "ColorMap.h"

GifMaker::GifMaker()
    : m_gifh(nullptr)
    , m_width(0)
    , m_height(0)
    , m_frame(0)
    , m_color_map(nullptr)
{}

GifMaker::~GifMaker()
{
    delete m_color_map;
}

bool GifMaker::open(const QString &out_file, const QImage *img_for_generating_color_map)
{
    qDebug() << "open" << QThread::currentThreadId();
    if(m_gifh)
    {
        close();
    }
    int error = 0;
    m_gifh = EGifOpenFileName(out_file.toUtf8().data(), 0, &error);
    if(!m_gifh)
    {
        qDebug() << "could not open new gif file, error" << error;
        return false;
    }
    EGifSetGifVersion(m_gifh, 1);

    delete m_color_map;
    m_color_map = new ColorMap(*img_for_generating_color_map);
    m_out_file = out_file;
    m_frame = 0;
    return true;
}
bool GifMaker::close()
{
    int error = 0;
    qDebug() << "EGifCloseFile" << QThread::currentThreadId();
    if(GIF_ERROR == EGifCloseFile(m_gifh, &error))
    {
        qDebug() << "EGifCloseFile failed.." << error;
        return false;
    }
    qDebug() << "EGifCloseFile ok";
    m_gifh = nullptr;
    m_width = 0;
    m_height = 0;
    m_frame = 0;
    return true;
}
bool GifMaker::addFirstFrame(const QImage &img, int loop_delay_ms)
{
    if(!m_gifh)
    {
        return false;
    }
    int bg_color = 0;
    ColorMap empty_color_map;
    auto color_map = m_color_map ? m_color_map->get() : empty_color_map.get();

    if(GIF_OK != EGifPutScreenDesc(m_gifh,
                                   img.width(),
                                   img.height(),
                                   color_map->ColorCount,
                                   bg_color,
                                   color_map))
    {
        qDebug() << "EGifPutScreenDesc failed" << GifErrorString(m_gifh->Error);
        return false;
    }
    m_width = img.width();
    m_height = img.height();
    addLoop(m_gifh);
    addGifFrame(m_gifh, img, loop_delay_ms);
    return true;
}
bool GifMaker::addFrame(const QImage &img, int loop_delay_ms)
{
    qDebug() << "addFrame" << QThread::currentThreadId();

    if(m_frame == 0)
    {
        return addFirstFrame(img, loop_delay_ms);
    }
    if(m_width == 0 || m_height == 0)
    {
        return false;
    }
    if(m_width != img.width())
    {
        return false;
    }
    if(m_height != img.height())
    {
        return false;
    }
    if(addGifFrame(m_gifh, img, loop_delay_ms) == GIF_OK)
    {
        return true;
    }
    return false;
}
bool GifMaker::write(const QList<QImage> png_files, const QString &out_file, int loop_delay_ms) const
{
    int error = 0;
    GifFileType *gifh = EGifOpenFileName(out_file.toUtf8().data(), 0, &error);
    if(!gifh)
    {
        qDebug() << "could not open new gif file, error" << error;
        return false;
    }
    EGifSetGifVersion(gifh, 1);

    int bg_color = 0;
    ColorMap map;
    auto color_map = map.get();

    int width = png_files.first().width();
    int height = png_files.first().height();
    if(GIF_OK != EGifPutScreenDesc(gifh,
                                   width, height,
                                   color_map->ColorCount,
                                   bg_color,
                                   color_map))
    {
        qDebug() << "EGifPutScreenDesc failed" << GifErrorString(gifh->Error);
        return false;
    }

    addLoop(gifh);
    for(const QImage &img : png_files)
    {
        addGifFrame(gifh, img, loop_delay_ms);
    }

    if(GIF_ERROR == EGifCloseFile(gifh, &error))
    {
        qDebug() << "EGifCloseFile failed." << error;
        return false;
    }

    return true;
}

bool GifMaker::addLoop(GifFileType *gifh) const
{
    char nsle[12] = "NETSCAPE2.0";
    if (EGifPutExtension(gifh, APPLICATION_EXT_FUNC_CODE, 11, nsle) == GIF_ERROR)
    {
        return false;
    }

    char subblock[3];
    subblock[0] = 1;
    subblock[1] = 0;
    subblock[2] = 0;
    if (EGifPutExtension(gifh, CONTINUE_EXT_FUNC_CODE, sizeof(subblock), subblock) == GIF_ERROR)
    {
        return false;
    }

    return true;
}

int GifMaker::addGifFrame(GifFileType *gifh, const QImage &img, int delay_msec) const
{
    QElapsedTimer t;
    t.start();
//    ColorMap map(img);
    ColorMap &map = *m_color_map;
    auto color_map = map.get();
    qint64 color_map_time = t.elapsed();

    int width = img.width();
    int height = img.height();

    static unsigned char
            ExtStr[4] = { 0x04, 0x00, 0x00, 0xff };

    if(delay_msec < 100)
    {
        delay_msec = 100;//100 ms is min possible delay
    }

    ExtStr[0] = 0x04;
    ExtStr[1] = delay_msec/10 % 256;
    ExtStr[2] = delay_msec/10 / 256;
    if(GIF_OK != EGifPutExtension(gifh, GRAPHICS_EXT_FUNC_CODE, 4, ExtStr))
    {
        qDebug() << "EGifPutExtension failed" << GifErrorString(gifh->Error);
        return GIF_ERROR;
    }

    int top = 0;
    int left = 0;
    int gif_interlace = 0;
    if(GIF_OK != EGifPutImageDesc(gifh,
                                  left, top,
                                  width, height,
                                  gif_interlace,
                                  color_map))
    {
        qDebug() << "EGifPutImageDesc failed" << GifErrorString(gifh->Error);
        return GIF_ERROR;
    }

    t.restart();
    for(int i = 0; i < height; ++i)
    {
        unsigned char line[width * 3];//TODO
        img.constScanLine(i);
        const unsigned int *rgb = (const unsigned int *)img.constScanLine(i);
        for(int j = 0; j < width; ++j)
        {
            line[j] = map.colorIndex(rgb[j]);
        }
        if(GIF_OK != EGifPutLine(gifh, line, width))
        {
            qDebug() << "EGifPutLine failed" << GifErrorString(gifh->Error);
            return GIF_ERROR;
        }
    }
    qint64 quantization_time = t.elapsed();
    qDebug() << "generated color map in " << color_map_time
             << "quantized frame " << m_frame
             << "in" << quantization_time << "ms";

    m_frame++;
    return GIF_OK;
}
