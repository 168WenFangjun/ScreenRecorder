#include "VideoFrameProducer.h"

#include <time.h>
#include <QDebug>
#include <QImage>
#include <QGuiApplication>
#include <QWindow>
#include <QScreen>
#include <QPainter>
#include <QtConcurrent/QtConcurrent>
#include <QMutexLocker>

#include "MouseInterface.h"
#include "Gif/GifMaker.h"
#include "WatermarkOpacityCalculator.h"

namespace {

static QImage &bltCursorOnImage(QImage &img, const QImage &cursor, const QPoint &pos)
{
    QPainter painter(&img);
    painter.drawImage(pos, cursor);
    painter.end();
    return img;
}

static AVFrame *qImageToAVFrame(const QImage &img, AVFrame *frame)
{
    SwsContext *sws_ctx = NULL;
    int swsFlags = 0
//            | SWS_LANCZOS
//            | SWS_BICUBIC
//            | SWS_BILINEAR
//            | SWS_FAST_BILINEAR
            | SWS_ACCURATE_RND;
    sws_ctx = sws_getCachedContext (sws_ctx,
                                     img.width(), img.height(), AV_PIX_FMT_BGRA,
                                     img.width(), img.height(), AV_PIX_FMT_YUV420P,
                                     swsFlags, NULL, NULL, NULL );
    uint8_t * inData[1] = { (uint8_t*) img.bits() };
    int inLinesize[1] = { 4 * img.width() }; // RGBA stride
//    int height =
            sws_scale(sws_ctx, inData, inLinesize,
                      0, img.height(),
                      frame->data, frame->linesize);
//    qDebug() << "#####"
//             << frame->linesize[0]
//             << frame->linesize[1]
//             << frame->linesize[2]
//             << height;
    sws_freeContext(sws_ctx);
    return frame;
}


}//namespace

VideoFrameProducer::VideoFrameProducer(MyEncoder &encoder,
                                       QMutex& encoder_m,
                                       LicenseType &licenseType,
                                       QWindow *window,
                                       QRect &region,
                                       QPoint &region_top_left,
                                       int &screen_idx,
                                       bool &capture_cursor,
                                       int &frame_rate,
                                       MouseInterface *mouse,
                                       GifMaker *gif_maker,
                                       QObject *parent)
    : QObject(parent)
    , m_encoder(encoder)
    , m_encoder_m(encoder_m)
    , m_licenseType(licenseType)
    , m_window(window)
    , m_region(region)
    , m_region_top_left(region_top_left)
    , m_frame_rate(frame_rate)
    , m_continue_recording(false)
    , m_screen_idx(screen_idx)
    , m_capture_cursor(capture_cursor)
    , m_mouse(mouse)
    , m_gif_maker(gif_maker)
    , m_screen(nullptr)
    , m_opacity_calculator(new WatermarkOpacityCalculator)
{
}

VideoFrameProducer::~VideoFrameProducer()
{
    delete m_opacity_calculator;
}

QList<QImage> VideoFrameProducer::getAvailableScreensAsThumbnails(const QList<QScreen *> &screens) const
{
    QList<QImage> thumbnails;
    for(auto screen : screens)
    {
        qDebug() << "screen"
                 << screen->geometry().x()
                 << screen->geometry().y();
        thumbnails.append(screen->grabWindow(0).
                          toImage().
                          scaledToWidth(200,
                                        Qt::TransformationMode::SmoothTransformation));
    }
    return thumbnails;
}

QImage VideoFrameProducer::getScreenShot(QScreen *screen, qreal watermark_opacity) const
{
    if (!screen)
    {
        return QImage();
    }
    //todo Does not work well on X11, use XGetImage instead!!
//    m_transparent_window->hide();
    QImage img = m_region.size().isNull() ?
                screen->grabWindow(0).toImage() :
                screen->grabWindow(0, m_region.left(), m_region.top(), m_region.width(), m_region.height()).toImage();
//    ((QWidget*)(m_transparent_window))->show();


//    static int i = 0;
//    img.save(QString("/home/oosman/Documents/1/screen-shot%1.png").arg(i));
//    ++i;
    img = applyWatermark(img, watermark_opacity);
    return img;
}

void VideoFrameProducer::startVideoRecording(QScreen *screen, int width, int height)
{
    qDebug() << "startVideoRecording";
    m_screen = screen;
    m_continue_recording = true;
    m_elapsed_since_first_frame.start();
    m_elapsed_since_last_frame.start();
    m_num_frames_encoded = 0;
    m_opacity_calculator->reset(time(0));
    QMetaObject::invokeMethod(this,
                              "enqueVideoSamples",
                              Qt::QueuedConnection);
}

void VideoFrameProducer::stopVideoRecording()
{
    qDebug() << "stopVideoRecording";
    m_continue_recording = false;
}

QImage& VideoFrameProducer::applyMouseCursor(QImage& img)
{
    if(!m_capture_cursor)
    {
        return img;
    }
    QPoint mouse_pos;
    QImage cursor = m_mouse->getMouseCursor(mouse_pos);
    if(cursor.isNull())
    {
        return img;
    }
//    qDebug() << "mouse" << mouse_pos
//             << "geom" << QGuiApplication::screens().at(m_screen_idx)->geometry();
    QRect screen_rect = QGuiApplication::screens().at(m_screen_idx)->geometry();
    if(!screen_rect.contains(mouse_pos))
    {
        return img;
    }
    mouse_pos.setX(mouse_pos.x() - screen_rect.x());
    mouse_pos.setY(mouse_pos.y() - screen_rect.y());

    if(!m_region.size().isNull())
    {
        QPoint region_top_left(m_region_top_left.rx(), m_region_top_left.ry());
        mouse_pos = mouse_pos - region_top_left;
    }
    img = bltCursorOnImage(img, cursor, mouse_pos);

    return img;
}

void VideoFrameProducer::writeFrame(const QImage &img, qint64 elapsed_ms)
{
    if(m_encoder.video_codec_id == AV_CODEC_ID_GIF)
    {
        m_gif_maker->addFrame(img, elapsed_ms);
    }
    else
    {
        AVFrame* frame = allocFrame(img.width(), img.height());
        frame = qImageToAVFrame(img, frame);
        frame->pts = calculateFrameTimestamp(m_encoder.start_time,
                                             m_encoder.video_st);
        frame->format = AV_PIX_FMT_YUV420P;//AV_PIX_FMT_BGR24?
        ::writeVideoFrame(&m_encoder, frame);
        freeFrame(frame);
    }

    m_num_frames_encoded++;
}

void VideoFrameProducer::enqueVideoSamples()
{
    if(!m_continue_recording)
    {
        emit stoppedVideoRecording();
        return;
    }
    qint64 elapsed_ms = m_elapsed_since_last_frame.elapsed();
    if(elapsed_ms < 1000 / m_frame_rate)
    {
        QMetaObject::invokeMethod(this,
                                  "enqueVideoSamples",
                                  Qt::QueuedConnection);
        return;
    }
    m_elapsed_since_last_frame.restart();

    qreal opacity = 0.0;

    if(m_licenseType != LicenseType_ScreenRecorder)
    {
        opacity = m_opacity_calculator->getWatermarkOpacity();
    }
    QImage img = getScreenShot(m_screen, opacity);
    img = applyMouseCursor(img);

    if(!img.isNull())
    {
        QMutexLocker lock(&m_encoder_m);
        writeFrame(img, elapsed_ms);
    }

    QMetaObject::invokeMethod(this,
                              "enqueVideoSamples",
                              Qt::QueuedConnection);
}

QImage &VideoFrameProducer::applyWatermark(QImage &destImage, qreal watermark_opacity) const
{
    if(watermark_opacity <= 0)
    {
        return destImage;
    }

    QImage srcImage = QImage(":/resources/watermark.png");
    int x = destImage.width() - srcImage.width();
    int y = destImage.height() - srcImage.height();
    QPoint destPos = QPoint(x, y);  // The location to draw the source image within the dest

    QPainter painter(&destImage);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.setOpacity(watermark_opacity);
    painter.drawImage(destPos, srcImage);
    painter.end();

    return destImage;
}

float VideoFrameProducer::getFrameRate() const
{
    float elapsed_s = m_elapsed_since_first_frame.elapsed() / 1000;
    float frame_rate = elapsed_s > 0 ? (m_num_frames_encoded / elapsed_s) : 0;
    return frame_rate;
}

