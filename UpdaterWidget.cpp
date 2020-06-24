#include <QDir>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QStandardPaths>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QSslError>
#include <QScreen>
#include <QUuid>

#include "UpdaterWidget.h"
#include "NetworkRequest.h"
#include "ui_UpdaterWidget.h"

#include "License.h"

extern "C" void mylog(const char *fmt, ...);

namespace
{
const char* getOS()
{
#if defined(Win32)
    return "win32";
#elif defined(Darwin)
    return "osx";
#elif defined(Linux)
    return "linux";
#endif
}

}//namespace

UpdaterWidget::UpdaterWidget(const QString app_name,
                             int app_id,
                             int version,
                             QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::UpdaterWidget)
    , m_continue_download(true)
    , m_downloading(false)
    , m_app_id(app_id)
    , m_app_version(version)
    , m_install_file_size(0)
    , m_app_name(app_name)
    , m_network(new NetworkRequest(this))
{
    ui->setupUi(this);
    ui->progressBarDownload->setMinimum(0);
    ui->progressBarDownload->setMaximum(100);
    ui->progressBarDownload->setValue(0);
    ui->progressBarDownload->setVisible(false);
}

UpdaterWidget::~UpdaterWidget()
{
    delete ui;
}

void UpdaterWidget::setCheckboxForCheckingUpdates(bool set)
{
    ui->checkBoxCheckForUpdates->setChecked(set);
}

bool UpdaterWidget::getCheckboxForCheckingUpdates() const
{
    return ui->checkBoxCheckForUpdates->isChecked();
}

void UpdaterWidget::receivedCheckForNewVersionReply(QNetworkReply* reply)
{
    QString data = reply->readAll();
    qDebug() << "check new version response:" << data;
    QJsonParseError error;
    auto obj = QJsonDocument::fromJson(data.toUtf8(), &error).object();
    if(error.error != QJsonParseError::ParseError::NoError)
    {
        qDebug() << "parse error in server reply:" << data
                 << "error" << error.errorString();
        return;
    }
    qDebug() << obj.toVariantMap() << "error" << error.errorString();
    QString install_file = obj.contains("file") ? obj.value("file").toString() : "";
    QString install_notes = obj.contains("notes") ? obj.value("notes").toString() : "";
    int install_file_size = obj.contains("size") ? obj.value("size").toInt() : 0;
    int new_version = obj.contains("version") ? obj.value("version").toInt() : 0;
    onReceivedVersionInfo(new_version, install_file, install_file_size, install_notes);
}

void UpdaterWidget::checkForNewVersion(QUuid &guid)
{
    QVariantMap map;
    map.insert("appid", m_app_id);
    map.insert("cmd", (int)License::Cmd::GET_VERSION);
    map.insert("os", getOS());
    map.insert("guid", guid.toString());
    map.insert("version", m_app_version);

    auto reply = m_network->post(QUrl("https://www.osletek.com/upgrade.php"),
                QJsonDocument(QJsonObject::fromVariantMap(map)).
                                    toJson(QJsonDocument::JsonFormat::Compact));
    connect(reply, &QNetworkReply::finished, [this,reply](){
        receivedCheckForNewVersionReply(reply);
        reply->deleteLater();
    });
}

QString UpdaterWidget::getDestinationPath(const QString &exe_file) const
{
    QString destination_path = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    destination_path += QDir::separator();

    QDir d(destination_path + m_app_name);
    if(!d.exists())
    {
        QDir d(destination_path);
        if(d.mkdir(m_app_name))
        {
            destination_path += m_app_name;
        }
    }
    else
    {
        destination_path += m_app_name;
    }
    destination_path += QDir::separator();
    destination_path += exe_file;

    return destination_path;
}

void UpdaterWidget::downloadInstallFile()
{
    QString destination_path = getDestinationPath(m_install_file);

    QFile f(destination_path);
    if(f.exists())
    {
        f.remove();
    }

    m_total_bytes_downloaded = 0;
    m_downloading = true;

    QVariantMap map;
    map.insert("appid", m_app_id);
    map.insert("cmd", (int)License::Cmd::GET_APP);
    map.insert("os", getOS());

    auto reply = m_network->post(QUrl("https://www.osletek.com/upgrade.php"),
                QJsonDocument(QJsonObject::fromVariantMap(map)).
                                    toJson(QJsonDocument::JsonFormat::Compact));
    connect(this, &UpdaterWidget::cancelDownload,
            reply, &QNetworkReply::abort);//TODO check if cancelDownload triggers finished
    connect(reply, &QNetworkReply::readyRead, [this,reply](){
        onReceivedFileData(reply->readAll());
    });
    connect(reply, &QNetworkReply::finished, [this,reply](){
        onReceivedFileData(reply->readAll());
        reply->deleteLater();
    });
}

void UpdaterWidget::onReceivedVersionInfo(int new_version, const QString &install_file, int install_file_size, const QString &install_notes)
{
    m_downloading = false;

    if(new_version > m_app_version)
    {
        ui->plainTextEdit->setPlainText(install_notes);
        m_install_file = install_file;
        m_install_file_size = install_file_size;

        ui->pushButtonDownload->setEnabled(true);

        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(QApplication::activeWindow(), "Upgrade",
              QString("New version %1 is available.\n\n%2\n\nDownload in background?").
                  arg(QString::number(new_version/100.0, '0', 2)).
                  arg(install_notes),
              QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes)
        {
            on_pushButtonDownload_clicked();
        }
    }
    else
    {
        ui->plainTextEdit->setPlainText("No new upgrade available.");
        m_install_file = "";
        m_install_file_size = 0;
    }
}

void UpdaterWidget::onReceivedFileData(const QByteArray &data)
{
    if(data.size() > 0)
    {
        m_total_bytes_downloaded += data.size();
        ui->progressBarDownload->setValue(m_total_bytes_downloaded * 100.0 / m_install_file_size);
        mylog("recvd data %i, of %i, %.0f%%", m_total_bytes_downloaded, m_install_file_size,
            m_total_bytes_downloaded * 100.0 / m_install_file_size);

        QString destination_path = getDestinationPath(m_install_file);
        FILE* fp = fopen(destination_path.toUtf8().data(), "a+b");
        fwrite(data.data(), 1, data.size(), fp);
        fclose(fp);
    }
    if(!m_continue_download)
    {
        emit cancelDownload();
    }
    if(m_total_bytes_downloaded >= m_install_file_size &&
            m_downloading)
    {
        downloadFinished();
    }
}

void UpdaterWidget::downloadFinished()
{
    ui->plainTextEdit->setPlainText("Download");
    ui->checkBoxCheckForUpdates->setEnabled(true);
    m_downloading = false;

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(QApplication::activeWindow(), "Upgrade",
                                  QString("File downloaded successfully.\n\n"
                                  "%1\n\n"
                                  "Do you want to upgrade now?").
                                  arg(getDestinationPath(m_install_file)),
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes)
    {
#if defined(Darwin)
        QStringList args;
        args <<  "attach" << getDestinationPath(m_install_file);
        QProcess::startDetached("hdiutil", args);
#elif defined(Linux)
        QDesktopServices::openUrl(QUrl::fromLocalFile(
                                      getDestinationPath("")));
#elif defined(Win32)
        QProcess::startDetached(getDestinationPath(m_install_file));
        QApplication::quit();
#endif//Q_OS_DARWIN
    }
}

void UpdaterWidget::on_checkBoxCheckForUpdates_clicked()
{
    ui->pushButtonDownload->setEnabled(false);
    ui->plainTextEdit->setPlainText("");
}

void UpdaterWidget::on_pushButtonDownload_clicked()
{
    if(!m_downloading)
    {
        ui->pushButtonDownload->setText("Cancel");
        ui->progressBarDownload->setVisible(true);
        ui->progressBarDownload->setValue(0);
        ui->checkBoxCheckForUpdates->setEnabled(false);
        m_continue_download = true;
        downloadInstallFile();
    }
    else
    {
        ui->pushButtonDownload->setText("Download");
        ui->progressBarDownload->setVisible(false);
        ui->progressBarDownload->setValue(0);
        ui->checkBoxCheckForUpdates->setEnabled(true);
        m_continue_download = false;
        m_downloading = false;
    }
}

