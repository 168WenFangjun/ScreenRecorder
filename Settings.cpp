#include "Settings.h"
#include "ui_Settings.h"

#include <QPushButton>
#include <QToolButton>
#include <QCheckBox>
#include <QRadioButton>
#include <QPainter>
#include <QButtonGroup>
#include <QDebug>
#include <QFormLayout>
#include <QFileDialog>
#include <QKeyEvent>
#include <QMetaEnum>
#include <QStandardPaths>

#include "AudioInInterface.h"

namespace {


QString toMultilineString(const QString &in)
{
    auto words = in.split(' ');
    QString out;
    int len = 0;
    for(auto word : words)
    {
        out.append(word);
        len = out.length();
        if(len < 10)
        {
            out.append(' ');
        }
        else
        {
            out.append('\n');
            len = 0;
        }
    }
    return out;
}
template<typename EnumType>
QString ToString(const EnumType& enumValue)
{
    const char* enumName = qt_getEnumName(enumValue);
    const QMetaObject* metaObject = qt_getEnumMetaObject(enumValue);
    if (metaObject)
    {
        const int enumIndex = metaObject->indexOfEnumerator(enumName);
        return QString("%1::%2::%3").arg(metaObject->className(), enumName, metaObject->enumerator(enumIndex).valueToKey(enumValue));
    }

    return QString("%1::%2").arg(enumName).arg(static_cast<int>(enumValue));
}

QString printableKeyText(QKeyEvent *keyEvent)
{
    QString text;
    if(keyEvent->modifiers() & Qt::ControlModifier)
    {
        if(text.size() > 0) { text.append("+"); }
        text.append("Ctrl");
    }
    if(keyEvent->modifiers() & Qt::ShiftModifier)
    {
        if(text.size() > 0) { text.append("+"); }
        text.append("Shift");
    }
    if(keyEvent->modifiers() & Qt::AltModifier)
    {
        if(text.size() > 0) { text.append("+"); }
        text.append("Alt");
    }
    QString key = QKeySequence(keyEvent->nativeVirtualKey()).toString();
    if(QKeySequence::fromString(
                QKeySequence(keyEvent->key()).toString())
            == keyEvent->key())
    {
        if(text.size() > 0) { text.append("+"); }
        text.append(key);
    }

    return text;
}


}
///////////////////////////////////////////////////////

TheButton::TheButton(QWidget *parent)
    : QToolButton(parent)
{}

TheButton::~TheButton()
{}

void TheButton::paintEvent(QPaintEvent *event)
{
    QToolButton::paintEvent(event);

    if(!isChecked())
    {
        return;
    }
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(0x5a, 0xc9, 0xd4), 10, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
    painter.drawRect(rect());
    painter.end();

}
///////////////////////////////////////////////////////

Settings::Settings(QList<HotKey> &hotkeys, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Settings)
    , m_hotkeys(hotkeys)
    , m_layout(nullptr)
    , m_screen_buttons_group(new QButtonGroup(this))
    , m_microphones_buttons_group(new QButtonGroup(this))
    , m_movie_format_buttons_group(new QButtonGroup(this))
    , m_screen_idx(0)
    , m_audio_idx(0)
{
    ui->setupUi(this);
    ui->lineEditHotKeyStartRecording->installEventFilter(this);
    ui->lineEditHotKeyScreenShot->installEventFilter(this);

    m_movie_format_buttons_group->addButton(ui->radioButtonMp4, 0);
    m_movie_format_buttons_group->addButton(ui->radioButtonOgg, 1);
    m_movie_format_buttons_group->addButton(ui->radioButtonMkv, 2);
    m_movie_format_buttons_group->addButton(ui->radioButtonGif, 3);
    connect(m_movie_format_buttons_group,
            static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked),
            [&](int idx){
        emit settingsChanged();
    });
    connect(m_screen_buttons_group,
            static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked),
            [&](int idx){
        m_screen_idx = idx;
        emit settingsChanged();
    });
    connect(m_microphones_buttons_group,
            static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked),
            [&](int idx){
        m_audio_idx = idx;
        emit settingsChanged();
    });
}

Settings::~Settings()
{
    delete m_layout;
    delete ui;
}

void Settings::addScreenThumbnails(const QList<QImage> &screens)
{
    for(QAbstractButton* button : m_screen_buttons_group->buttons())
    {
        m_screen_buttons_group->removeButton(button);
        delete button;
    }
    if(screens.count() < 2)
    {
        ui->screens->hide();
        return;
    }

    int idx = 0;
    for(auto screen : screens)
    {
        auto button = new TheButton(this);
        button->setIcon(QIcon(QPixmap::fromImage(screen)));
        button->setIconSize(QSize(screen.width(), screen.height()));
        button->setCheckable(true);
        ((QGridLayout*)ui->screens->layout())->addWidget(button, idx / 2, idx % 2);
        m_screen_buttons_group->addButton(button);
        m_screen_buttons_group->setId(button, idx++);
    }
    m_screen_buttons_group->button(m_screen_idx)->setChecked(true);
}

void Settings::addMicrophoneThumbnails(const QList<AudioDevice> &microphones)
{
    for(QAbstractButton* button : m_microphones_buttons_group->buttons())
    {
        m_microphones_buttons_group->removeButton(button);
        delete button;
    }
    if(microphones.count() < 2)
    {
        ui->microphones->hide();
        return;
    }

    QSize largest_size(100, 100);
    int idx = 0;
    QList<TheButton*> list;
    for(auto microphone : microphones)
    {
        auto button = new TheButton(this);
        list.append(button);
        button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        button->setIcon(QIcon(QPixmap::fromImage(microphone.m_img)));
        button->setText(toMultilineString(microphone.m_name));
        button->setIconSize(QSize(microphone.m_img.width(), microphone.m_img.height()));
        if(button->size().width() > largest_size.width() )
        {
            largest_size.setWidth(button->size().width());
        }
        if(button->size().height() > largest_size.height() )
        {
            largest_size.setHeight(button->size().height());
        }
        button->setCheckable(true);
        ((QGridLayout*)ui->microphones->layout())->addWidget(button, idx / 2, idx % 2);
        m_microphones_buttons_group->addButton(button);
        m_microphones_buttons_group->setId(button, idx++);
    }
    for(auto button : list)
    {
        button->setMaximumSize(largest_size);
        button->setMinimumSize(largest_size);
    }
    m_microphones_buttons_group->button(m_audio_idx)->setChecked(true);
}

int Settings::getAudioDeviceIndex() const
{
    return m_audio_idx;
}


int Settings::getScreenIndex() const
{
    return m_screen_idx;
}

QString Settings::getMovieFormat() const
{
    if(ui->radioButtonMp4->isChecked()) return "mp4";
    else if(ui->radioButtonOgg->isChecked()) return "ogg";
    else if(ui->radioButtonMkv->isChecked()) return "mkv";
    else if(ui->radioButtonGif->isChecked()) return "gif";
}

void Settings::on_toolButtonCaptureCursor_clicked()
{
    setCaptureCursorButtonState();
    emit settingsChanged();
}

bool Settings::getCaptureCursor() const
{
    return !(ui->toolButtonCaptureCursor->isChecked());
}

int Settings::getFrameRate() const
{
    return ui->spinBoxFps->value();
}

QString Settings::getOutputDirectory() const
{
    return ui->lineEditDirectory->text();
}

void Settings::on_toolButtonBrowse_clicked()
{
    auto path = QFileDialog::getExistingDirectory(this,
                                                  tr("Where do you want to save your file?"),
                                                  ui->lineEditDirectory->text().isEmpty() ?
                                                      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) :
                                                      ui->lineEditDirectory->text());
    if(path.isEmpty())
    {
        path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    ui->lineEditDirectory->setText(path);
    emit settingsChanged();
}

bool Settings::eventFilter(QObject *target, QEvent *event)
{
    if (event->type() != QEvent::KeyPress)
    {
        return QWidget::eventFilter(target, event);
    }
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
    if(keyEvent->modifiers() != 0 &&
            (keyEvent->modifiers() & Qt::ControlModifier) != Qt::ControlModifier)
    {
        ui->labelHotKeyMessage->setText("Only Ctrl key is supported");
        return QWidget::eventFilter(target, event);
    }
    qDebug() << "settings"
             << keyEvent->nativeVirtualKey()
             << keyEvent->nativeScanCode()
             << keyEvent->nativeModifiers();
    ui->labelHotKeyMessage->clear();
    if (target == ui->lineEditHotKeyStartRecording)
    {
        m_keyEvent.insert(HotKey::Action::Record, MyKeyEvent(
                              keyEvent->nativeVirtualKey(),
                              keyEvent->nativeModifiers(),
                              printableKeyText(keyEvent)));
        ui->lineEditHotKeyStartRecording->setText(printableKeyText(keyEvent));
        return true;//we are handling this event, parent should not
    }
    else if(target == ui->lineEditHotKeyScreenShot)
    {
        m_keyEvent.insert(HotKey::Action::ScreenShot, MyKeyEvent(
                              keyEvent->nativeVirtualKey(),
                              keyEvent->nativeModifiers(),
                              printableKeyText(keyEvent)));
        ui->lineEditHotKeyScreenShot->setText(printableKeyText(keyEvent));
        return true;
    }
    return false;//allow parent to process events
}

void Settings::setAudioDeviceIndex(int idx)
{
    m_audio_idx = idx;
    if(m_microphones_buttons_group->buttons().count() > idx)
    {
        m_microphones_buttons_group->button(idx)->setChecked(true);
    }
}

void Settings::setScreenIndex(int idx)
{
    m_screen_idx = idx;
    if(m_screen_buttons_group->buttons().count() > idx)
    {
        m_screen_buttons_group->button(idx)->setChecked(true);
    }
}

void Settings::setMovieFormat(const QString &format)
{
    if(format == "mp4") ui->radioButtonMp4->setChecked(true);
    else if(format == "ogg") ui->radioButtonOgg->setChecked(true);
    else if(format == "mkv") ui->radioButtonMkv->setChecked(true);
    else if(format == "gif") ui->radioButtonGif->setChecked(true);
}

void Settings::setCaptureCursor(bool capture)
{
    ui->toolButtonCaptureCursor->setChecked(!capture);
    setCaptureCursorButtonState();
}

void Settings::setCaptureCursorButtonState()
{
    (ui->toolButtonCaptureCursor->isChecked()) ?
        ui->toolButtonCaptureCursor->setIcon(QIcon(":/resources/no-mouse.png")) :
        ui->toolButtonCaptureCursor->setIcon(QIcon(":/resources/mouse.png"));
}

void Settings::setFrameRate(int frame_rate)
{
    ui->spinBoxFps->setValue(frame_rate);
}

void Settings::setOutputDirectory(const QString &output_directory)
{
    ui->lineEditDirectory->setText(output_directory);
}

void Settings::setHotKeys()
{
    for(auto hotkey : m_hotkeys)
    {
        switch(hotkey.m_action)
        {
        case HotKey::Action::Record:
            ui->lineEditHotKeyStartRecording->setText(hotkey.m_name);
            break;
        case HotKey::Action::ScreenShot:
            ui->lineEditHotKeyScreenShot->setText(hotkey.m_name);
            break;
        }
    }
}

void Settings::setVolume(int volume)
{
    ui->volumeSlider->setValue(volume);
    ui->groupBoxVolume->setTitle(QString("Volume %1%").arg(volume));
}

int Settings::getVolume() const
{
    return ui->volumeSlider->value();
}

void Settings::on_lineEditHotKeyStartRecording_editingFinished()
{
    updateHotKeys();
}

void Settings::on_lineEditHotKeyScreenShot_editingFinished()
{
    updateHotKeys();
}

void Settings::updateHotKeys()
{
    emit clearHotKeys();
    m_hotkeys.clear();
    if(m_keyEvent.contains(HotKey::Action::ScreenShot))
    {
        m_hotkeys.append(HotKey(
                HotKey::Action::ScreenShot,
                m_keyEvent[HotKey::Action::ScreenShot].m_modifiers,
                m_keyEvent[HotKey::Action::ScreenShot].m_key,
                m_keyEvent[HotKey::Action::ScreenShot].m_name));
    }
    if(m_keyEvent.contains(HotKey::Action::Record))
    {
        m_hotkeys.append(HotKey(
                HotKey::Action::Record,
                m_keyEvent[HotKey::Action::Record].m_modifiers,
                m_keyEvent[HotKey::Action::Record].m_key,
                m_keyEvent[HotKey::Action::Record].m_name));
    }

}
void Settings::on_pushButtonResetHotKeys_clicked()
{
    ui->lineEditHotKeyStartRecording->clear();
    ui->lineEditHotKeyScreenShot->clear();
    emit clearHotKeys();
}

void Settings::on_volumeSlider_sliderMoved(int position)
{
    ui->groupBoxVolume->setTitle(QString("Volume %1%").arg(position));
}
