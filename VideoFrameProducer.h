#pragma once

#include <QObject>
#include <QMutex>
#include <QElapsedTimer>

#include "LicenseType.h"
#include "ffwrite.h"
#include "../DS/circularbuffer.h"

class GifMaker;
class QScreen;
class QWindow;
class MouseInterface;
class WatermarkOpacityCalculator;
class VideoFrameProducer : public QObject
{
    Q_OBJECT
public:
    VideoFrameProducer(MyEncoder& encoder,
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
                       QObject *parent = nullptr);
    virtual ~VideoFrameProducer();
    QImage getScreenShot(QScreen *screen, qreal watermark_opacity) const;
    QList<QImage> getAvailableScreensAsThumbnails(const QList<QScreen *> &screens) const;
    float getFrameRate() const;

signals:
    void writeVideoFrame();
    void stoppedVideoRecording();

public slots:
    void startVideoRecording(QScreen *screen, int width, int height);
    void stopVideoRecording();

protected:
    MyEncoder &m_encoder;
    QMutex &m_encoder_m;
    LicenseType &m_licenseType;
    QWindow *m_window;
    QRect &m_region;
    QPoint &m_region_top_left;
    bool m_continue_recording;
    QElapsedTimer m_elapsed_since_first_frame;
    QElapsedTimer m_elapsed_since_last_frame;
    qint64 m_num_frames_encoded;
    int &m_frame_rate;
    int &m_screen_idx;
    bool &m_capture_cursor;
    MouseInterface *m_mouse;
    GifMaker *m_gif_maker;
    QScreen *m_screen;
    WatermarkOpacityCalculator *m_opacity_calculator;

    QImage &applyWatermark(QImage &destImage, qreal watermark_opacity) const;
    Q_INVOKABLE void enqueVideoSamples();
    QImage &applyMouseCursor(QImage &img);
    void writeFrame(const QImage &img, qint64 elapsed_ms);
};
