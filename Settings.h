#pragma once

#include <QWidget>
#include <QToolButton>
#include <QKeyEvent>

#include "KeyInterface.h"

namespace Ui {
class Settings;
}

class TheButton : public QToolButton
{
public:
    explicit TheButton(QWidget *parent = Q_NULLPTR);
    ~TheButton();
private:
    virtual void paintEvent(QPaintEvent *event);
};


class AudioDevice;
class QHBoxLayout;
class QVBoxLayout;
class QAbstractButton;
class QButtonGroup;
class Settings : public QWidget
{
    Q_OBJECT

signals:
    void settingsChanged();
    void clearHotKeys();
public:
    explicit Settings(QList<HotKey> &hotkeys, QWidget *parent = 0);
    ~Settings();

    void addScreenThumbnails(const QList<QImage> &screens);
    void addMicrophoneThumbnails(const QList<AudioDevice> &microphones);
    int getAudioDeviceIndex() const;
    int getScreenIndex() const;
    QString getMovieFormat() const;
    bool getCaptureCursor() const;
    int getFrameRate() const;
    QString getOutputDirectory() const;

    void setAudioDeviceIndex(int idx);
    void setScreenIndex(int idx);
    void setMovieFormat(const QString &format);
    void setCaptureCursor(bool capture);
    void setFrameRate(int frame_rate);
    void setOutputDirectory(const QString &output_directory);
    void setHotKeys();
    void setVolume(int volume);
    int getVolume() const;

private slots:
    void on_toolButtonCaptureCursor_clicked();
    void on_toolButtonBrowse_clicked();
    void on_lineEditHotKeyStartRecording_editingFinished();
    void on_lineEditHotKeyScreenShot_editingFinished();
    void on_pushButtonResetHotKeys_clicked();

    void on_volumeSlider_sliderMoved(int position);

protected:
    bool eventFilter(QObject *target, QEvent *event);
    void setCaptureCursorButtonState();
    void updateHotKeys();

private:
    struct MyKeyEvent
    {
        quint32 m_key;
        quint32 m_modifiers;
        QString m_name;
        MyKeyEvent(quint32 key, quint32 modifiers, const QString &name)
            : m_key(key), m_modifiers(modifiers), m_name(name) {}
        MyKeyEvent()
            : m_key(-1), m_modifiers(-1), m_name("") {}
    };
    Ui::Settings *ui;
    QHBoxLayout *m_layout;
    QButtonGroup *m_screen_buttons_group;
    QButtonGroup *m_microphones_buttons_group;
    QButtonGroup *m_movie_format_buttons_group;
    int m_screen_idx;
    int m_audio_idx;
    QList<HotKey> &m_hotkeys;
    QMap<HotKey::Action, MyKeyEvent> m_keyEvent;
};
