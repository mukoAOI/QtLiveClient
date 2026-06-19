#include "core/bilibili_utils.h"

#include "core/constants.h"

#include <QRegularExpression>

namespace BiliLive {

QNetworkRequest makeBiliRequest(const QUrl& url) {
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QString::fromLatin1(kUserAgent));
    request.setRawHeader("Referer", kReferer);
    return request;
}

qint64 parseRoomId(const QString& input) {
    const QString trimmed = input.trimmed();
    if (trimmed.isEmpty()) {
        return 0;
    }

    static const QRegularExpression urlRe(
        QStringLiteral(R"(live\.bilibili\.com/(?:blanc/)?(\d+))"));
    const QRegularExpressionMatch urlMatch = urlRe.match(trimmed);
    if (urlMatch.hasMatch()) {
        return urlMatch.captured(1).toLongLong();
    }

    static const QRegularExpression idRe(QStringLiteral(R"(^(\d+)$)"));
    const QRegularExpressionMatch idMatch = idRe.match(trimmed);
    if (idMatch.hasMatch()) {
        return idMatch.captured(1).toLongLong();
    }

    return 0;
}

QString normalizeLiveUrl(qint64 roomId) {
    return QStringLiteral("https://live.bilibili.com/%1").arg(roomId);
}

QString liveStatusText(int status) {
    switch (status) {
    case 1:
        return QStringLiteral("直播中");
    case 2:
        return QStringLiteral("轮播中");
    default:
        return QStringLiteral("未开播");
    }
}

}  // namespace BiliLive
