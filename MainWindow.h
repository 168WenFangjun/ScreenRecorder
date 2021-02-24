#pragma once

#include <QButtonGroup>
#include <QMainWindow>
#include <QMutex>
#include <QSettings>
#include <QUuid>
#include <stdio.h>

#include "ffwrite.h"

#include "LicenseType.h"
#include "KeyInterface.h"

namespace Ui {
class MainWindow;
}

class AudioInInterface;
class AudioFrameProducer;
class BufferWriter;
class CameraWidget;
class CrossCursorWidget;
class GifMaker;
class VideoFrameProducer;
class LicenseWidget;
class LicenseWidgetDialog;
class TransparentMaximizedWindow;
class QScreen;
struct AVFrame;
class AboutWidget;
class Settings;
class TabDialog;
class MouseInterface;
class KeyInterface;
class UpdaterWidget;
class WindowEnumeratorInterface;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void showCrossCursorDialog(const QString &title, const QString &lable, const QImage &img);

signals:
    void enqueAudioSamples(void *audio_samples_p,
                           unsigned long num_samples,
                           unsigned long num_bytes_per_sample,
                           short channels,
                           unsigned long samples_per_frame);
    void startRecording(QScreen* screen, int width, int height);
    void stopRecording();

protected:
    void captureFullScreen();
    static int writeFunction(void *opaque, uint8_t *buf, int buf_size);
    static int64_t seekFunction(void *opaque, int64_t offset, int whence);
    void setMovieFormat(const QString &format);
    QString getMovieFormat() const;
    void initializeCameraWidget();
    void scrubScreenCaptureModeOperation();

private slots:
    void updateWindowTitleAndButton();
    void stoppedVideoRecording();
    void stoppedAudioRecording();
    void stoppedRecording();
    void saveSettings();
    void mouseTracker();
    void hotKeyPressed(quint32 key, quint32 modifiers);
    void clearHotKeys();
    void showTransparentWindowOverlay();
    void selectRegionForCapture(const QPoint &top_left, const QRect &border_rect);
    void selectWindowForCapture();
    void captureRegionSizeChanged(float top_left_x, float top_left_y, float bottom_right_x, float bottom_right_y);
    void audioData(void *audio_samples_p, unsigned long num_samples, unsigned long num_bytes_per_sample, short channels);
    void on_pushButtonRecord_clicked();
    void on_pushButtonScreenShot_clicked();
    void on_pushButtonSelectRegion_clicked();
    void on_pushButtonCamera_clicked();
    void on_pushButtonMicrophone_clicked();
    void on_pushButtonAbout_clicked();

    void on_pushButtonSelectFullScreen_clicked();

    void on_pushButtonSelectWindow_clicked();

private:
    Ui::MainWindow *ui;
    QTimer *m_window_update_timer;
    MyEncoder m_encoder;
    QString m_output_file;
    QRect m_region;
    QPoint m_region_top_left;
    enum CaptureMode
    {
        FullScreen,
        Region,
        Window,
        Max
    };
    CaptureMode m_capture_mode;
    bool m_record_audio;
    bool m_recording_audio;
    bool m_recording_video;
    TransparentMaximizedWindow *m_transparent_window;
    CrossCursorWidget *m_cross_cursor_window;
    AudioInInterface *m_audio_in;
    time_t m_time_capture_started;
    MouseInterface *m_mouse;
    KeyInterface *m_key;
    QMutex m_encoder_m;
    LicenseType m_licenseType;
    LicenseWidget *m_license_widget;
    GifMaker *m_gif_maker;
    AudioFrameProducer *m_audio_producer;
    VideoFrameProducer *m_video_producer;
    QThread *m_audio_producer_thread;
    QThread *m_video_producer_thread;
    QThread *m_hotkey_capture_thread;
    int m_screen_idx;
    int m_audio_idx;
    bool m_capture_cursor;
    int m_frame_rate;
    QString m_output_dir;
    QList<HotKey> m_hotkeys;
    int m_audio_volume;
    Settings* m_settings_widget;
    QSettings m_persistent_settings;
    AboutWidget *m_about;
    TabDialog *m_settings_tab_dialog;
    QUuid m_guid;
    UpdaterWidget *m_updater_widget;
    CameraWidget *m_camera_widget;
    WindowEnumeratorInterface *m_window_enumerator;
    QTimer *m_mouse_tracker;
    QList<QScreen *> m_screens;
    QButtonGroup m_capture_mode_buttons;
    std::unique_ptr<BufferWriter> m_ffp[2];

    void prepareToRecord();
    void captureRegion();
    void captureWindow();
    virtual void closeEvent(QCloseEvent *);
    void openFolder();
    void screenShotThread();
    void resetQueues();
    void initializeLicenseWidget();
    void setupSettingsTabDialog();
    void initializeSettingsWidget();
    void loadSettings();
    void updateMuteButtonIcon();
    void initAvailableScreens();
};
