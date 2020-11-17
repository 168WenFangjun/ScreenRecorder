#include <functional>
#include <QCloseEvent>
#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QGridLayout>
#include <QList>
#include <QNetworkReply>
#include <QPainter>
#include <QScreen>
#include <QStandardPaths>
#include <QtConcurrent/QtConcurrent>
#include <QTimer>
#include <QUrl>
#include <QWindow>


#include <stdio.h>
#include <time.h>

#include "AudioInInterface.h"
#include "InterfaceFactory.h"
#include "AudioFrameProducer.h"
#include "CameraWidget.h"
#include "CrossCursorWidget.h"
#include "FFMpegRunnerDialog.h"
#include "Gif/GifMaker.h"
#include "VideoFrameProducer.h"
#include "AboutWidget.h"
#include "../DS/circularbuffer.h"
#include "LicenseWidget.h"
#include "MainWindow.h"
#include "MouseInterface.h"
#include "Settings.h"
#include "TransparentMaximizedWindow.h"
#include "TabDialog.h"
#include "UpdaterWidget.h"
#include "WindowEnumeratorInterface.h"
#include "ui_MainWindow.h"

extern "C" int64_t 	av_gettime (void);
extern "C" int64_t av_gettime_relative(void);

namespace
{
enum
{
    CMD_GET_VERSION=1,
    CMD_GET_APP,
    CMD_CHECK_LICENSE
};

static const QString APP_NAME("ScreenRecorder");
static const int APP_ID = 2203;
static const int APP_VERSION = 230;

static CIRCULAR_Q <AVFrame*>audio_samples_q(32);
static CIRCULAR_Q <AVFrame*>video_samples_q(32);

static QString getFileSizeDisplayString(const QString &output_file)
{
    QFileInfo info(output_file);
    int64_t bytes = info.size();
    float display_bytes = bytes;
    QString display_units = "Bytes";
    int64_t gb = bytes/1024/1024/1024;
    int64_t mb = (bytes - gb*1024)/1024/1024;
    int64_t kb = (bytes - mb*1024)/1024;
    if(gb > 0)
    {
        display_bytes = bytes/1024.0/1024.0/1024.0;
        display_units = "GB";
    }
    else if(mb > 0)
    {
        display_bytes = bytes/1024.0/1024.0;
        display_units = "MB";
    }
    else if(kb > 0)
    {
        display_bytes = bytes/1024.0;
        display_units = "KB";
    }
    QString ret = QString("%1 %2").
        arg(QString::number(display_bytes, 'f', 3)).
        arg(display_units);
    return ret;
}

static QString getTimeElapsedDisplayString(time_t time_capture_started)
{
    float diff = difftime(time(0), time_capture_started);
    long hours = diff/60.0/60.0;
    long minutes = (diff - hours*60.0*60.0)/60.0;
    long seconds = diff - minutes*60.0 - hours*60.0*60.0;

    QString ret = QString("%1:%2:%3").
//                   arg(QString::number(hours, 'f', 0)).
//                    arg(QString::number(minutes, 'f', 0)).
//                    arg(QString::number(seconds, 'f', 0));
                   arg(hours,2,10,QChar('0')).
                   arg(minutes,2,10,QChar('0')).
                   arg(seconds,2,10,QChar('0'));
    return ret;
}

static QString getTimestamp()
{
    time_t t = time(0);
    char tbuffer [128];
    tm * timeinfo = localtime ( &t );
    strftime (tbuffer, 80,"%Y.%m.%d-%H.%M.%S", timeinfo);

    return tbuffer;
}

QString getLicenseWidgetLabel()
{
    return "You are using the demo version.\n\n"
            "Would you purchase a license\n"
            "so we can continue providing this software?";
}

extern "C" void mylog(const char *fmt, ...)
{
    char tbuffer [128];
    time_t t = time(0);
    tm * timeinfo = localtime ( &t );
    strftime (tbuffer, 80,"%y-%m-%d %H:%M:%S", timeinfo);

    char buffer1[1024];
    char buffer2[1024+128];
    va_list args;
    va_start(args, fmt);
    vsprintf (buffer1, fmt, args);
    sprintf(buffer2, "%s - %s\n", tbuffer, buffer1);
    QString *msg = new QString(buffer2);
    qDebug() << *msg;

    static QString output_dir(QStandardPaths::writableLocation(
                                  QStandardPaths::DocumentsLocation) + "/screen-recorder.log");
    FILE* fp = fopen(output_dir.toUtf8().data(), "a+t");
    if(fp) {
        fwrite(buffer2, strlen(buffer2), 1, fp);
        fclose(fp);
    }

    va_end(args);
}

static int maxSamplesPerAudioFrame(enum AVCodecID audio_codec_id, int default_num)
{
    switch(audio_codec_id)
    {
    case AV_CODEC_ID_VORBIS:
        return 64;
    case AV_CODEC_ID_AAC:
    default:
        return default_num;
    }
}


}//namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_window_update_timer(new QTimer(this))
    , m_capture_mode(CaptureMode::FullScreen)
    , m_transparent_window(nullptr)
    , m_cross_cursor_window(nullptr)
    , m_record_audio(true)
    , m_recording_audio(false)
    , m_recording_video(false)
    , m_licenseType(LicenseType_Invalid)
    , m_mouse(InterfaceFactory::createMouseInterface(this))
    , m_key(InterfaceFactory::createKeyInterface())
    , m_audio_in(nullptr)
    , m_gif_maker(new GifMaker)
    , m_audio_producer(new AudioFrameProducer(m_encoder, m_encoder_m))
    , m_video_producer(new VideoFrameProducer(m_encoder,
                                              m_encoder_m,
                                              m_licenseType,
                                              windowHandle(),
                                              m_region,
                                              m_region_top_left,
                                              m_screen_idx,
                                              m_capture_cursor,
                                              m_frame_rate,
                                              m_mouse,
                                              m_gif_maker))
    , m_audio_producer_thread(new QThread(this))
    , m_video_producer_thread(new QThread(this))
    , m_hotkey_capture_thread(new QThread(this))
    , m_screen_idx(0)
    , m_audio_idx(0)
    , m_capture_cursor(true)
    , m_frame_rate(60)
    , m_settings_widget(new Settings(m_hotkeys))
    , m_about(new AboutWidget)
    , m_output_dir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
    , m_audio_volume(30)
    , m_settings_tab_dialog(nullptr)
    , m_persistent_settings(QString("%1/ScreenRecorder.ini").
                          arg(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)),
                          QSettings::IniFormat)
    , m_guid(QUuid::createUuid())
    , m_updater_widget(new UpdaterWidget(APP_NAME, APP_ID, APP_VERSION, this))
    , m_camera_widget(new CameraWidget(this))
    , m_window_enumerator(InterfaceFactory::createWindowEnumeratorInterface())
    , m_mouse_tracker(new QTimer(this))
{
    qDebug() << "MainWindow" << QThread::currentThreadId()
             << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    ui->setupUi(this);

    m_audio_in = InterfaceFactory::createAudioInterface();
    m_encoder.write_func = writeFunction;
    m_encoder.read_func = nullptr;
    m_encoder.seek_func = seekFunction;
    m_encoder.opaque_io_data = (void*)this;
    m_encoder.video_codec_id = AV_CODEC_ID_H264;//AV_CODEC_ID_MPEG4;
    m_encoder.audio_codec_id = AV_CODEC_ID_AAC;

    connect(m_window_update_timer, &QTimer::timeout, this, &MainWindow::updateWindowTitleAndButton);
    connect(m_video_producer, &VideoFrameProducer::stoppedVideoRecording,
            this, &MainWindow::stoppedVideoRecording);
    connect(m_audio_producer, &AudioFrameProducer::stoppedAudioRecording,
            this, &MainWindow::stoppedAudioRecording);
    setWindowIcon(QIcon(":/resources/blue-footed-booby-with-camera.png"));
    initAvailableScreens();
    loadSettings();
    //after loadSettings:
    initializeLicenseWidget();
    initializeSettingsWidget();
    setupSettingsTabDialog();
    m_updater_widget->checkForNewVersion(m_guid);

    m_audio_producer_thread->start();
    m_audio_producer->moveToThread(m_audio_producer_thread);
    connect(this, &MainWindow::enqueAudioSamples,
            m_audio_producer, &AudioFrameProducer::enqueAudioSamples,
            Qt::QueuedConnection);
    connect(m_audio_producer, &AudioFrameProducer::destroyed,
            m_audio_producer_thread, &QThread::quit);
    connect(m_audio_in, &AudioInInterface::audioData,
            this, &MainWindow::audioData, Qt::QueuedConnection);

    m_video_producer_thread->start();
    m_video_producer->moveToThread(m_video_producer_thread);
    connect(this, &MainWindow::startRecording,
            m_video_producer, &VideoFrameProducer::startVideoRecording,
            Qt::QueuedConnection);
    connect(this, &MainWindow::startRecording,
            m_audio_producer, &AudioFrameProducer::startAudioRecording,
            Qt::QueuedConnection);
    connect(this, &MainWindow::stopRecording,
            m_video_producer, &VideoFrameProducer::stopVideoRecording,
            Qt::QueuedConnection);
    connect(this, &MainWindow::stopRecording,
            m_audio_producer, &AudioFrameProducer::stopAudioRecording,
            Qt::QueuedConnection);
    connect(m_video_producer, &VideoFrameProducer::destroyed,
            m_video_producer_thread, &QThread::quit);

    m_hotkey_capture_thread->start();
    m_key->moveToThread(m_hotkey_capture_thread);
    connect(m_key, &KeyInterface::destroyed,
            m_hotkey_capture_thread, &QThread::quit);
    connect(m_key, &KeyInterface::hotKeyPressed,
            this, &MainWindow::hotKeyPressed,
            Qt::QueuedConnection);

    m_mouse_tracker->setInterval(500);
    m_mouse_tracker->setSingleShot(false);
    connect(m_mouse_tracker, &QTimer::timeout,
            this, &MainWindow::mouseTracker);
    initializeCameraWidget();
    m_capture_mode_buttons.addButton(ui->pushButtonSelectFullScreen);
    m_capture_mode_buttons.addButton(ui->pushButtonSelectRegion);
    m_capture_mode_buttons.addButton(ui->pushButtonSelectWindow);
    m_capture_mode_buttons.setExclusive(true);
}

MainWindow::~MainWindow()
{
    scrubScreenCaptureModeOperation();
    delete m_audio_in;
    delete m_about;
    delete m_settings_widget;
    delete m_settings_tab_dialog;
    delete ui;
    m_audio_producer->deleteLater();
    m_audio_producer_thread->quit();
    m_audio_producer_thread->wait();
    m_audio_producer_thread->deleteLater();

    m_video_producer->deleteLater();
    m_video_producer_thread->quit();
    m_video_producer_thread->wait();
    m_video_producer_thread->deleteLater();

    delete m_gif_maker;
    m_key->deleteLater();
    m_hotkey_capture_thread->quit();
    m_hotkey_capture_thread->wait();
    m_hotkey_capture_thread->deleteLater();

    m_mouse->deleteLater();
}

void MainWindow::initAvailableScreens()
{
    m_screens = QGuiApplication::screens();
    std::sort(m_screens.begin(), m_screens.end(),
              [](QScreen *a, QScreen *b){
        return (a->geometry().x() != b->geometry().x()) ?
                    a->geometry().x() < b->geometry().x() :
                    a->geometry().y() < b->geometry().y();
    });
}

int MainWindow::writeFile(uint8_t *buf, int buf_size)
{
    return fwrite(buf, 1, buf_size, m_ffp);
}

int64_t MainWindow::seekFile(int64_t offset, int whence)
{
    return fseek(m_ffp, offset, whence);
}

int MainWindow::writeFunction(void *opaque, uint8_t *buf, int buf_size)
{
    MainWindow *self = (MainWindow*)opaque;
    return self->writeFile(buf, buf_size);
}

int64_t MainWindow::seekFunction(void *opaque, int64_t offset, int whence)
{
    MainWindow *self = (MainWindow*)opaque;
    return self->seekFile(offset, whence);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_camera_widget->deleteLater();
    m_hotkey_capture_thread->quit();
    m_hotkey_capture_thread->wait();
    if(m_window_update_timer->isActive())
    {
        qDebug() << "closeEvent emit stopVideoRecording";
        emit stopRecording();
        event->ignore();
        QTimer::singleShot (500, this, SLOT(close()));
    }
    else
    {
        qDebug() << "closeEvent, not recording";
        stoppedVideoRecording();

        resetQueues();
        event->accept();
    }
}

void MainWindow::scrubScreenCaptureModeOperation()
{
    m_mouse_tracker->stop();
    delete m_transparent_window;
    m_transparent_window = nullptr;
    delete m_cross_cursor_window;
    m_cross_cursor_window = nullptr;
}

void MainWindow::showCrossCursorDialog(const QString &title, const QString &lable, const QImage &img)
{
    scrubScreenCaptureModeOperation();
    m_cross_cursor_window = new CrossCursorWidget(title, lable, img, this);
    m_cross_cursor_window->show(pos().x() + width()/2 - m_cross_cursor_window->width()/2,
                                pos().y() + height()/2 - m_cross_cursor_window->height()/2);
    connect(m_cross_cursor_window, &CrossCursorWidget::done,
            this, &MainWindow::showTransparentWindowOverlay);
}

void MainWindow::captureFullScreen()
{
    if(!ui->pushButtonSelectFullScreen->isChecked())
    {
        ui->pushButtonSelectFullScreen->setChecked(true);
    }
    scrubScreenCaptureModeOperation();
    m_capture_mode = CaptureMode::FullScreen;
    qDebug() << "reset region";
    m_region = QRect();
    m_mouse_tracker->stop();
}

void MainWindow::captureRegion()
{
    if(m_capture_mode == CaptureMode::Region)
    {
        captureFullScreen();
        return;
    }
    m_capture_mode = CaptureMode::Region;
    showCrossCursorDialog("Capture Region",
                          "1) Click on + below to start\n"
                          "2) Move cursor to top-left corner of a region and click\n"
                          "4) Move cursor to bottom-right corner and click again\n",
                          QImage(":/resources/select-region.png")
                          );
}

void MainWindow::captureWindow()
{
    if(m_capture_mode == CaptureMode::Window)
    {
        captureFullScreen();
        return;
    }
    m_capture_mode = CaptureMode::Window;
    qDebug() << "reset region";
    m_region = QRect();
    showCrossCursorDialog("Capture Window",
                          "1) Click on + below to start\n"
                          "2) Move the cursor to title bar of a window\n",
                          QImage(":/resources/select-window.png")
                          );
}

void MainWindow::mouseTracker()
{
    int screen_idx = 0;
    for(auto screen : m_screens)
    {
        if(screen->geometry().contains(QCursor::pos()))
        {
            break;
        }
        screen_idx++;
    }
    qDebug() << "mouseTracker screen_idx" << screen_idx;
    m_transparent_window->moveToScreen(m_screens[screen_idx]);
    m_screen_idx = screen_idx;
}

void MainWindow::showTransparentWindowOverlay()
{
    scrubScreenCaptureModeOperation();
    m_mouse_tracker->start();
    QImage screen_shot = m_video_producer->getScreenShot(m_screens[m_screen_idx], 0);

    m_transparent_window = new TransparentMaximizedWindow(this);
    m_transparent_window->show(screen_shot.width(), screen_shot.height(), m_screens[m_screen_idx]);
    connect(m_transparent_window, &TransparentMaximizedWindow::selectRegionForCapture,
            this, &MainWindow::selectRegionForCapture);
    connect(m_transparent_window, &TransparentMaximizedWindow::captureRegionSizeChanged,
            this, &MainWindow::captureRegionSizeChanged);
}

void MainWindow::selectRegionForCapture(const QPoint &top_left, const QRect &border_rect)
{
    if(m_capture_mode == CaptureMode::Region)
    {
        m_region_top_left = top_left;
        m_region = border_rect;
        qDebug() << "CaptureMode::Region ("
                 << border_rect.topLeft()
                 << "), ("
                 << border_rect.bottomRight() << "),";
    }
    else if(m_capture_mode == CaptureMode::Window)
    {
        selectWindowForCapture();
    }
    m_mouse_tracker->stop();
    m_persistent_settings.setValue("screen_index", m_screen_idx);
    m_settings_widget->setScreenIndex(m_screen_idx);
}

void MainWindow::selectWindowForCapture()
{
    QPoint mouse_pos = QCursor::pos();

    scrubScreenCaptureModeOperation();
    QImage screen_shot = m_video_producer->getScreenShot(m_screens[m_screen_idx], 0);
    m_region = m_window_enumerator->windowRectWithPoint(
                mouse_pos.x()/* + m_screens[m_screen_idx]->geometry().left()*/,
                mouse_pos.y()/* + m_screens[m_screen_idx]->geometry().top()*/);
    m_region_top_left = QPoint(m_region.left(), m_region.top());

    m_transparent_window = new TransparentMaximizedWindow(this);
    m_transparent_window->show(screen_shot.width(), screen_shot.height(), m_screens[m_screen_idx]);
    m_transparent_window->startCapture(QPoint(m_region.left() - m_screens[m_screen_idx]->geometry().left(),
                                              m_region.top() - m_screens[m_screen_idx]->geometry().top()),
                                       QPoint(m_region.left() - m_screens[m_screen_idx]->geometry().left() + m_region.width(),
                                              m_region.top() - m_screens[m_screen_idx]->geometry().top() + m_region.height()));
    qDebug() << "selectWindowForCapture"
             << "clicked at"
             << mouse_pos.x()
             << mouse_pos.y()
             << "screen_idx" << m_screen_idx
             << "top_left" << m_region_top_left
             << "region" << m_region
             << "screen (" << screen_shot.width()
             << "," << screen_shot.height() << ")";
}

void MainWindow::updateWindowTitleAndButton()
{
    static int count = 0;
    ui->pushButtonRecord->setIcon(QIcon(
        (count++ % 2 == 0) ?
            ":/resources/recording-led-on.png" : ":/resources/recording-led-off.png"));

    setWindowTitle(QString("Screen Recorder: %1, %2, %3 fps").
                   arg(getTimeElapsedDisplayString(m_time_capture_started)).
                   arg(getFileSizeDisplayString(m_output_file)).
                   arg(m_video_producer->getFrameRate(), 0, 'f', 1)
                   );
}

void MainWindow::stoppedAudioRecording()
{
    qDebug() << "stoppedAudioCapture";
    m_recording_audio = false;
    stoppedRecording();
}

void MainWindow::stoppedVideoRecording()
{
    qDebug() << "stoppedVideoCapture";
    m_recording_video = false;
    stoppedRecording();
}

void MainWindow::stoppedRecording()
{
    if(m_recording_audio)
    {
        return;
    }
    if(m_recording_video)
    {
        return;
    }
    if(!m_window_update_timer->isActive())
    {
        return;
    }
    m_window_update_timer->stop();
    if(m_encoder.video_codec_id == AV_CODEC_ID_GIF)
    {
        m_gif_maker->close();
    }
    else
    {
        deinitializeEncoder(&m_encoder);
        fclose(m_ffp);
        m_ffp = nullptr;
    }
    qDebug() << "getMovieFormat" << getMovieFormat();
    if(getMovieFormat() == "mp4")
    {
//        int video_duration = difftime(time(0), m_time_capture_started);
//        auto ffmpeg_runner_dlg = new FFMpegRunnerDialog;
//        ffmpeg_runner_dlg->init(m_output_file, video_duration, m_encoder.width, m_encoder.height);
//        emit ffmpeg_runner_dlg->start();
//        ffmpeg_runner_dlg->exec();
//        delete ffmpeg_runner_dlg;
    }
    qDebug() << "deinit";
    ui->pushButtonRecord->setIcon(QIcon(":/resources/record-button.png"));
    ui->statusBar->showMessage(QString("Saved %1").arg(m_output_file));
    mylog(QString("Saved %1").arg(m_output_file).toUtf8().data());
    ui->pushButtonSelectRegion->setEnabled(true);
    ui->pushButtonMicrophone->setEnabled(true);
    ui->pushButtonAbout->setEnabled(true);

    openFolder();
}

void MainWindow::captureRegionSizeChanged(float top_left_x, float top_left_y,
                                          float bottom_right_x, float bottom_right_y)
{
    ui->statusBar->showMessage(QString("(%1,%2) -> (%3,%4) / [%5, %6]").
                               arg(top_left_x).
                               arg(top_left_y).
                               arg(bottom_right_x).
                               arg(bottom_right_y).
                               arg(bottom_right_x - top_left_x + 1).
                               arg(bottom_right_y - top_left_y + 1));
}

void MainWindow::resetQueues()
{
    while(!video_samples_q.isEmpty())
    {
        AVFrame *frame = video_samples_q.remove();
        freeFrame(frame);
    }
    while(!audio_samples_q.isEmpty())
    {
        AVFrame *frame = audio_samples_q.remove();
        freeFrame(frame);
    }
    m_encoder.frames_per_second = 60;
    m_encoder.video_pts = 0;
    m_encoder.audio_pts = 0;
}

void MainWindow::audioData(void *audio_samples_p, unsigned long num_samples, unsigned long num_bytes_per_sample, short channels)
{
    emit enqueAudioSamples(
                audio_samples_p,
                num_samples,
                num_bytes_per_sample,
                channels,
                maxSamplesPerAudioFrame(
                    m_encoder.audio_codec_id, num_samples)
                );
}

void MainWindow::prepareToRecord()
{
    resetQueues();
    QImage img = m_video_producer->getScreenShot(m_screens[m_screen_idx], 0);
    m_encoder.width = img.width();
    m_encoder.height = img.height();
    m_output_file = QString("%1%2screen-recording-%3.%4").
            arg(m_output_dir).arg(QDir::separator()).
            arg(getTimestamp()).
            arg(getMovieFormat());

    m_time_capture_started = time(0);
    m_window_update_timer->start(250);
    ui->pushButtonRecord->setIcon(QIcon(":/resources/recording-led-on.png"));
    ui->statusBar->showMessage(QString("Recording %1").arg(m_output_file));
    mylog(QString("Recording %1").arg(m_output_file).toUtf8().data());
    ui->pushButtonSelectRegion->setEnabled(false);
    ui->pushButtonMicrophone->setEnabled(false);
    ui->pushButtonAbout->setEnabled(false);

    if(m_encoder.video_codec_id == AV_CODEC_ID_GIF)
    {
        m_recording_video = true;
        m_gif_maker->open(m_output_file.toUtf8().data(), &img);
        emit startRecording(m_screens[m_screen_idx], img.width(), img.height());
        return;
    }
    if(m_record_audio &&
            m_audio_in->Init() &&
            m_audio_in->Open(m_audio_idx, m_audio_volume))
    {
        m_encoder.audio_bitrate =  m_audio_in->GetBitRate();
        m_encoder.audio_sample_rate = m_audio_in->GetSampleRate();
        m_encoder.audio_num_channels = m_audio_in->GetNumChannels();
        m_encoder.audio_frame_size = m_audio_in->GetBufferSize();
        setMovieFormat(m_persistent_settings.value("movie_format", "mp4").toString());
        qDebug() << "bitrate " << m_encoder.audio_bitrate;
        qDebug() << "sample_rate " << m_encoder.audio_sample_rate;
        qDebug() << "frames_per_second " << m_encoder.frames_per_second;
        qDebug() << "num_channels " << m_encoder.audio_num_channels;
        qDebug() << "frame_size " << m_encoder.audio_frame_size;
        m_recording_audio = true;
    }
    else
    {
        m_recording_audio = false;
        if(m_audio_in && m_audio_in->GetError()[0])
        {
            ui->statusBar->showMessage(m_audio_in->GetError());
            mylog("Error opening audio: %s", m_audio_in->GetError());
        }
    }
    m_ffp = fopen(m_output_file.toUtf8().data(), "wb");
    initializeEncoder(m_output_file.toUtf8().data(), m_recording_audio, &m_encoder);
    if(m_recording_audio)
    {
        m_audio_in->Start();
    }

    m_recording_video = true;
    emit startRecording(m_screens[m_screen_idx], img.width(), img.height());
}

void MainWindow::on_pushButtonRecord_clicked()
{
    if(m_window_update_timer->isActive())
    {
        if(m_audio_in)
        {
            m_audio_in->Stop();
            m_audio_in->Close();
        }
        emit stopRecording();
    }
    else
    {
        prepareToRecord();
    }
}

void MainWindow::on_pushButtonScreenShot_clicked()
{
    m_output_file = QString("%1%2screen-shot-%3.png").
            arg(m_output_dir).arg(QDir::separator()).
            arg(getTimestamp());
    m_video_producer->getScreenShot(m_screens[m_screen_idx], 0)
            .save(m_output_file);
    ui->statusBar->showMessage(QString("Saved %1").arg(m_output_file));
    mylog(QString("Error opening audio: %1").
          arg(m_audio_in ? QString(m_audio_in->GetError()) : "none").toUtf8().data());
    openFolder();
}

void MainWindow::on_pushButtonCamera_clicked()
{
    m_camera_widget->isVisible() ? m_camera_widget->hide() : m_camera_widget->show();
}


void MainWindow::on_pushButtonSelectFullScreen_clicked()
{
    captureFullScreen();
}

void MainWindow::on_pushButtonSelectRegion_clicked()
{
    captureRegion();
}

void MainWindow::on_pushButtonSelectWindow_clicked()
{
    captureWindow();
}

void MainWindow::openFolder()
{
/*
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    QString filetypes = "*.mp4 *.ogg";
    dialog.setNameFilter(tr(QString("Images (%1);;Any (*.*)").arg(filetypes).toUtf8().data()));
    dialog.setViewMode(QFileDialog::Detail);

    QStringList fileNames;
    if (dialog.exec())
    {
        fileNames = dialog.selectedFiles();
    }
    qDebug() << fileNames;
*/
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_output_dir));
}

void MainWindow::on_pushButtonMicrophone_clicked()
{
    m_record_audio = !m_record_audio;
    updateMuteButtonIcon();
}

void MainWindow::updateMuteButtonIcon()
{
    ui->pushButtonMicrophone->setIcon(QIcon(
        m_record_audio ? ":resources/microphone-on-button.png" :
                  ":resources/microphone-off-button.png"));
    m_persistent_settings.setValue("mute", m_record_audio);
}

void MainWindow::initializeLicenseWidget()
{
    m_license_widget = new LicenseWidget(APP_NAME, APP_ID, APP_VERSION, m_guid,
                                         QString("http://www.osletek.com/?a=order&&fileid=%1&&guid=%2")
                                                     .arg(APP_ID).arg(m_guid.toString()),
                                         this);
    m_license_widget->hide();
    m_license_widget->checkLicense(getLicenseWidgetLabel(),
                        [this](int licenseType){
                          m_licenseType = (LicenseType)licenseType;
    });
}

void MainWindow::setupSettingsTabDialog()
{
    QList<QPair<QString, QWidget*>> tabs;
    tabs.append(QPair<QString, QWidget*>("Settings", m_settings_widget));
    tabs.append(QPair<QString, QWidget*>("Update", m_updater_widget));
    tabs.append(QPair<QString, QWidget*>("License", m_license_widget));
    tabs.append(QPair<QString, QWidget*>("About", m_about));
    m_settings_tab_dialog = new TabDialog(tabs);
    connect(m_settings_tab_dialog, &QDialog::finished,
            this, &MainWindow::saveSettings);
    connect(m_settings_widget, &Settings::settingsChanged,
            this, &MainWindow::saveSettings);
    connect(m_settings_widget, &Settings::clearHotKeys,
            this, &MainWindow::clearHotKeys);
}

void MainWindow::on_pushButtonAbout_clicked()
{
    clearHotKeys();
    m_hotkeys.clear();
    m_settings_widget->setVolume(m_audio_volume);
    m_settings_tab_dialog->show();
}

void MainWindow::setMovieFormat(const QString &format)
{
    if(format == "mp4")
    {
        m_encoder.video_codec_id = AV_CODEC_ID_H264;//AV_CODEC_ID_MPEG4;
        m_encoder.audio_codec_id = AV_CODEC_ID_AAC;
    }
    else if(format == "ogg")
    {
        m_encoder.video_codec_id = AV_CODEC_ID_THEORA;
        m_encoder.audio_codec_id = AV_CODEC_ID_VORBIS;
    }
    else if(format == "mkv")
    {
        m_encoder.video_codec_id = AV_CODEC_ID_VP8;
        m_encoder.audio_codec_id = AV_CODEC_ID_VORBIS;
    }
    else if(format == "gif")
    {
        m_encoder.video_codec_id = AV_CODEC_ID_GIF;
        m_encoder.audio_codec_id = AV_CODEC_ID_NONE;
    }
}

QString MainWindow::getMovieFormat() const
{
    if(m_encoder.video_codec_id == AV_CODEC_ID_VP8 &&
            m_encoder.audio_codec_id == AV_CODEC_ID_VORBIS)
    {
        return "mkv";
    }
    else if((m_encoder.video_codec_id == AV_CODEC_ID_MPEG4 ||
            m_encoder.video_codec_id == AV_CODEC_ID_H264) &&
                m_encoder.audio_codec_id == AV_CODEC_ID_AAC)
    {
        return "mp4";
    }
    else if(m_encoder.video_codec_id == AV_CODEC_ID_THEORA &&
                m_encoder.audio_codec_id == AV_CODEC_ID_VORBIS)
    {
        return "ogg";
    }
    else if(m_encoder.video_codec_id == AV_CODEC_ID_GIF)
    {
        return "gif";
    }
    return "";
}

void MainWindow::initializeCameraWidget()
{
    m_camera_widget->setWindowFlags(Qt::Popup);
    m_camera_widget->setParent(QApplication::activeWindow());
}

void MainWindow::loadSettings()
{
    m_guid = m_persistent_settings.value("guid", m_guid).toUuid();
    m_screen_idx = m_persistent_settings.value("screen_index", 0).toInt();
    m_audio_idx = m_persistent_settings.value("audio_idx", 0).toInt();
    setMovieFormat(m_persistent_settings.value("movie_format", "mp4").toString());
    m_capture_cursor = m_persistent_settings.value("capture_cursor", true).toBool();
    m_frame_rate = m_persistent_settings.value("frame_rate", 60).toInt();
    m_output_dir = m_persistent_settings.value("output_dir",
                                               QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
    if(m_output_dir.isEmpty() || !QFile(m_output_dir).exists())
    {
        m_output_dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        m_persistent_settings.setValue("output_dir", m_output_dir);
    }
    m_record_audio = m_persistent_settings.value("mute", true).toBool();
    updateMuteButtonIcon();
    m_hotkeys.clear();
    m_persistent_settings.beginGroup("hotkeys");
    if(m_persistent_settings.value(QString("%1/modifier").arg(HotKey::Action::Record), 0).toInt() != 0)
    {
        HotKey hotkey = HotKey(
                         HotKey::Action::Record,
                         m_persistent_settings.value(QString("%1/modifier").arg(HotKey::Action::Record), 0).toInt(),
                         m_persistent_settings.value(QString("%1/key").arg(HotKey::Action::Record), 0).toInt(),
                         m_persistent_settings.value(QString("%1/name").arg(HotKey::Action::Record), 0).toString());
        m_hotkeys.insert(HotKey::Action::Record, hotkey);
        emit m_key->registerHotKey(hotkey.m_key, hotkey.m_modifiers);
    }
    if(m_persistent_settings.value(QString("%1/modifier").arg(HotKey::Action::ScreenShot), 0).toInt() != 0)
    {
        HotKey hotkey = HotKey(
                         HotKey::Action::ScreenShot,
                         m_persistent_settings.value(QString("%1/modifier").arg(HotKey::Action::ScreenShot), 0).toInt(),
                         m_persistent_settings.value(QString("%1/key").arg(HotKey::Action::ScreenShot), 0).toInt(),
                         m_persistent_settings.value(QString("%1/name").arg(HotKey::Action::ScreenShot), 0).toString());
        m_hotkeys.insert(HotKey::Action::ScreenShot, hotkey);
        emit m_key->registerHotKey(hotkey.m_key, hotkey.m_modifiers);
    }
    m_persistent_settings.endGroup();
    m_audio_volume = m_persistent_settings.value("volume", m_audio_volume).toInt();
    m_updater_widget->setCheckboxForCheckingUpdates(
                m_persistent_settings.value("automatic_check_for_updates", true).toBool());
}

void MainWindow::saveSettings()
{
    m_screen_idx = m_settings_widget->getScreenIndex();
    m_audio_idx = m_settings_widget->getAudioDeviceIndex();
    setMovieFormat(m_settings_widget->getMovieFormat());
    m_capture_cursor = m_settings_widget->getCaptureCursor();
    m_frame_rate = m_settings_widget->getFrameRate();
    m_output_dir = m_settings_widget->getOutputDirectory();

    m_persistent_settings.clear();

    m_persistent_settings.setValue("guid", m_guid);
    m_persistent_settings.setValue("screen_index", m_screen_idx);
    m_persistent_settings.setValue("audio_idx", m_audio_idx);
    m_persistent_settings.setValue("movie_format", m_settings_widget->getMovieFormat());
    m_persistent_settings.setValue("capture_cursor", m_capture_cursor);
    m_persistent_settings.setValue("frame_rate", m_frame_rate);
    m_persistent_settings.setValue("output_dir", m_output_dir);
    m_persistent_settings.setValue("mute", m_record_audio);

    m_persistent_settings.beginGroup("hotkeys");
    for(HotKey &hotkey : m_hotkeys)
    {
        m_persistent_settings.beginGroup(QString::number(hotkey.m_action));
        m_persistent_settings.setValue("key", QString::number(hotkey.m_key));
        m_persistent_settings.setValue("modifier", QString::number(hotkey.m_modifiers));
        m_persistent_settings.setValue("name", hotkey.m_name);
        m_persistent_settings.endGroup();

        qDebug() << "registerHotKey"
                 << "action" << hotkey.m_action
                 << "key" << hotkey.m_key
                 << "modifiers" << hotkey.m_modifiers;
        emit m_key->registerHotKey(hotkey.m_key, hotkey.m_modifiers);
    }

    m_persistent_settings.endGroup();

    m_audio_volume = m_settings_widget->getVolume();
    m_persistent_settings.setValue("volume", m_audio_volume);
    m_persistent_settings.setValue("automatic_check_for_updates", m_updater_widget->getCheckboxForCheckingUpdates());
}

void MainWindow::initializeSettingsWidget()
{
    m_settings_widget->addScreenThumbnails(m_video_producer->getAvailableScreensAsThumbnails(m_screens));
    auto audio_devices = m_audio_in->availableAudioDevices(true);
    for(auto &audio_device : audio_devices)
    {
        audio_device.m_img = QImage(":/resources/microphone.png");
    }
    m_settings_widget->addMicrophoneThumbnails(audio_devices);
    m_settings_widget->setOutputDirectory(m_output_dir);
    m_settings_widget->setScreenIndex(m_screen_idx);
    m_settings_widget->setAudioDeviceIndex(m_audio_idx);
    m_settings_widget->setMovieFormat(getMovieFormat());
    m_settings_widget->setCaptureCursor(m_capture_cursor);
    m_settings_widget->setFrameRate(m_frame_rate);
    m_settings_widget->setHotKeys();
    m_settings_widget->setVolume(m_audio_volume);
}

void MainWindow::clearHotKeys()
{
    for(HotKey &hotkey : m_hotkeys)
    {
        emit m_key->unRegisterHotKey(hotkey.m_key, hotkey.m_modifiers);
    }
    m_hotkeys.clear();
    saveSettings();
}

void MainWindow::hotKeyPressed(quint32 key, quint32 modifiers)
{
//    qDebug() << "hotKeyPressed" << key << modifiers;
    for(auto hotkey : m_hotkeys)
    {
        if(hotkey.m_key == key && hotkey.m_modifiers == modifiers)
        {
            switch(hotkey.m_action)
            {
            case HotKey::Action::Record:
                on_pushButtonRecord_clicked();
                break;
            case HotKey::Action::ScreenShot:
                on_pushButtonScreenShot_clicked();
                break;
            }
        }
    }
}
