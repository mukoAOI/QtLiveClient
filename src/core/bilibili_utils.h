#pragma once

#include <QNetworkRequest>
#include <QString>
#include <QUrl>

namespace BiliLive {

QNetworkRequest makeBiliRequest(const QUrl& url);
qint64 parseRoomId(const QString& input);
QString normalizeLiveUrl(qint64 roomId);
QString liveStatusText(int status);

}  // namespace BiliLive
