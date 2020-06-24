#pragma once

#include <QWidget>

class QNetworkAccessManager;
class QNetworkReply;
class QSslError;
class NetworkRequest;
class QUuid;

namespace Ui {
class UpdaterWidget;
}

class UpdaterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit UpdaterWidget(const QString app_name, int app_id, int version, QWidget *parent = nullptr);
    ~UpdaterWidget();

    void setCheckboxForCheckingUpdates(bool set);
    bool getCheckboxForCheckingUpdates() const;
    void checkForNewVersion(QUuid &guid);

protected slots:
    void on_pushButtonDownload_clicked();
    void on_checkBoxCheckForUpdates_clicked();

signals:
    void cancelDownload();

protected:
    Ui::UpdaterWidget *ui;
    bool m_continue_download;
    bool m_downloading;
    int m_total_bytes_downloaded;
    int m_app_id;
    int m_app_version;
    QString m_install_file;
    int m_install_file_size;
    const QString m_app_name;
    NetworkRequest *m_network;

    void downloadInstallFile();
    QString getDestinationPath(const QString &exe_file) const;
    void downloadFinished();
    void receivedCheckForNewVersionReply(QNetworkReply* reply);
    void onReceivedVersionInfo(int new_version, const QString &install_file, int install_file_size, const QString &install_notes);
    void onReceivedFileData(const QByteArray &data);
};
