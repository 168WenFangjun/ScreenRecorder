#include "NetworkRequest.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

NetworkRequest::NetworkRequest(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{

}

QNetworkReply *NetworkRequest::post(const QUrl &url, const QByteArray &data) const
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
//    config.setProtocol(QSsl::SslV3);//not working on LInux
    config.setProtocol(QSsl::AnyProtocol);
    request.setSslConfiguration(config);

    auto reply = m_network->post(request, data);
    connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
            [reply](QNetworkReply::NetworkError err){
        qDebug() << "error" << err
                 << "status code" << reply->attribute( QNetworkRequest::HttpStatusCodeAttribute).toInt()
                 << reply->errorString();
    });
    connect(reply, &QNetworkReply::sslErrors, [this,reply](const QList<QSslError> &err){
        qDebug() << "SSL error" << err;
        reply->ignoreSslErrors();//TODO for testing
    });
    return reply;
}
