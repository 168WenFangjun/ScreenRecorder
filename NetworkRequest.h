#pragma once

#include <QObject>

class QUrl;
class QNetworkAccessManager;
class QNetworkReply;
class NetworkRequest : public QObject
{
public:
    NetworkRequest(QObject *parent);
    QNetworkReply *post(const QUrl &url, const QByteArray &data) const;

protected:
    QNetworkAccessManager *m_network;
};


