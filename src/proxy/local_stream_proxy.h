#pragma once

#include <QList>
#include <QObject>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;
class QTcpServer;
class QTcpSocket;

namespace BiliLive {

class LocalStreamProxy : public QObject {
    Q_OBJECT

public:
    explicit LocalStreamProxy(QNetworkAccessManager* nam, QObject* parent = nullptr);

    QUrl start(const QString& upstreamUrl);
    void stop();

private:
    void pipeUpstream(QTcpSocket* client);

    QNetworkAccessManager* nam_ = nullptr;
    QTcpServer* server_ = nullptr;
    QString upstreamUrl_;
    QList<QNetworkReply*> activeReplies_;
};

}  // namespace BiliLive
