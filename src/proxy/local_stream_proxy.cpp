#include "proxy/local_stream_proxy.h"

#include "core/constants.h"

#include <QHostAddress>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTcpServer>
#include <QTcpSocket>

namespace BiliLive {

LocalStreamProxy::LocalStreamProxy(QNetworkAccessManager* nam, QObject* parent)
    : QObject(parent), nam_(nam) {}

QUrl LocalStreamProxy::start(const QString& upstreamUrl) {
    stop();
    upstreamUrl_ = upstreamUrl;

    server_ = new QTcpServer(this);
    connect(server_, &QTcpServer::newConnection, this, [this]() {
        QTcpSocket* client = server_->nextPendingConnection();
        connect(client, &QTcpSocket::readyRead, this, [this, client]() {
            if (client->property("reqDone").toBool()) {
                return;
            }
            client->setProperty("reqDone", true);
            client->readAll();
            pipeUpstream(client);
        });
    });
    if (!server_->listen(QHostAddress::LocalHost, 0)) {
        return {};
    }
    return QUrl(QStringLiteral("http://127.0.0.1:%1/live.flv").arg(server_->serverPort()));
}

void LocalStreamProxy::stop() {
    const QList<QNetworkReply*> replies = activeReplies_;
    activeReplies_.clear();
    for (QNetworkReply* reply : replies) {
        reply->abort();
        reply->deleteLater();
    }
    if (server_) {
        server_->close();
        server_->deleteLater();
        server_ = nullptr;
    }
}

void LocalStreamProxy::pipeUpstream(QTcpSocket* client) {
    QNetworkRequest req{QUrl(upstreamUrl_)};
    req.setHeader(QNetworkRequest::UserAgentHeader, QString::fromLatin1(kUserAgent));
    req.setRawHeader("Referer", kReferer);

    QNetworkReply* reply = nam_->get(req);
    activeReplies_.append(reply);

    connect(reply, &QNetworkReply::metaDataChanged, client, [reply, client]() {
        if (client->property("hdrSent").toBool()) {
            return;
        }
        const int status =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 0) {
            return;
        }
        if (status != 200) {
            client->write("HTTP/1.1 502 Bad Gateway\r\nConnection: close\r\n\r\n");
            client->disconnectFromHost();
            return;
        }
        client->write(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: video/x-flv\r\n"
            "Connection: close\r\n\r\n");
        client->setProperty("hdrSent", true);
        const QByteArray pending = reply->readAll();
        if (!pending.isEmpty()) {
            client->write(pending);
        }
    });

    connect(reply, &QNetworkReply::readyRead, client, [reply, client]() {
        if (!client->property("hdrSent").toBool()) {
            return;
        }
        while (reply->bytesAvailable() > 0 &&
               client->bytesToWrite() < kProxyMaxClientBuffer) {
            const QByteArray chunk = reply->read(kProxyChunkBytes);
            if (chunk.isEmpty()) {
                break;
            }
            client->write(chunk);
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, client]() {
        activeReplies_.removeAll(reply);
        reply->deleteLater();
        client->disconnectFromHost();
    });
    connect(client, &QTcpSocket::disconnected, reply, &QNetworkReply::abort);
}

}  // namespace BiliLive
