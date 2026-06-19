#include "ui/live_stream_tile.h"

#include "core/bilibili_utils.h"
#include "core/constants.h"
#include "proxy/local_stream_proxy.h"

#include <QAudioOutput>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMediaPlayer>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QVideoWidget>

namespace BiliLive {

LiveStreamTile::LiveStreamTile(qint64 roomId, const QString& inputUrl,
                               QNetworkAccessManager* network, QWidget* parent)
    : QFrame(parent), roomId_(roomId), inputUrl_(inputUrl), network_(network),
      streamQuality_(kQualityOrigin) {
    setFrameShape(QFrame::StyledPanel);
    setMinimumSize(320, 240);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);

    auto* header = new QHBoxLayout();
    titleLabel_ = new QLabel(QStringLiteral("房间 %1").arg(roomId_), this);
    titleLabel_->setStyleSheet(QStringLiteral("font-weight: bold;"));
    header->addWidget(titleLabel_, 1);

    audioCheck_ = new QCheckBox(QStringLiteral("声音"), this);
    audioCheck_->setChecked(false);
    header->addWidget(audioCheck_);

    volumeSlider_ = new QSlider(Qt::Horizontal, this);
    volumeSlider_->setRange(0, 100);
    volumeSlider_->setValue(80);
    volumeSlider_->setFixedWidth(88);
    volumeSlider_->setToolTip(QStringLiteral("音量"));
    header->addWidget(volumeSlider_);

    volumeLabel_ = new QLabel(QStringLiteral("80%"), this);
    volumeLabel_->setFixedWidth(34);
    volumeLabel_->setStyleSheet(QStringLiteral("color: #666; font-size: 11px;"));
    header->addWidget(volumeLabel_);

    closeBtn_ = new QPushButton(QStringLiteral("删除"), this);
    closeBtn_->setFixedWidth(56);
    header->addWidget(closeBtn_);
    layout->addLayout(header);

    videoWidget_ = new QVideoWidget(this);
    videoWidget_->setAttribute(Qt::WA_DontCreateNativeAncestors, true);
    videoWidget_->setMinimumSize(280, 158);
    layout->addWidget(videoWidget_, 1);

    infoLabel_ = new QLabel(QStringLiteral("正在连接…"), this);
    infoLabel_->setWordWrap(true);
    infoLabel_->setStyleSheet(QStringLiteral("color: #555; font-size: 11px;"));
    layout->addWidget(infoLabel_);

    streamProxy_ = new LocalStreamProxy(network_, this);
    player_ = new QMediaPlayer(this);
    audioOutput_ = new QAudioOutput(this);
    audioOutput_->setVolume(0.0);
    player_->setAudioOutput(audioOutput_);
    player_->setVideoOutput(videoWidget_);

    connect(closeBtn_, &QPushButton::clicked, this, [this]() { emit closeRequested(this); });
    connect(audioCheck_, &QCheckBox::toggled, this, [this](bool enabled) {
        applyVolume();
        if (enabled && volumeSlider_->value() == 0) {
            setVolumePercent(80);
        }
    });
    connect(volumeSlider_, &QSlider::valueChanged, this, &LiveStreamTile::setVolumePercent);
    connect(player_, &QMediaPlayer::errorOccurred, this, &LiveStreamTile::onPlayerError);
    videoWidget_->installEventFilter(this);
}

bool LiveStreamTile::eventFilter(QObject* watched, QEvent* event) {
    if (watched == videoWidget_ && event->type() == QEvent::MouseButtonDblClick) {
        emit exclusiveToggleRequested(this);
        return true;
    }
    return QFrame::eventFilter(watched, event);
}

void LiveStreamTile::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit exclusiveToggleRequested(this);
    }
    QFrame::mouseDoubleClickEvent(event);
}

qint64 LiveStreamTile::roomId() const { return roomId_; }

QString LiveStreamTile::inputUrl() const { return inputUrl_; }

bool LiveStreamTile::isAudioEnabled() const { return audioCheck_->isChecked(); }

int LiveStreamTile::volumePercent() const { return volumeSlider_->value(); }

void LiveStreamTile::setAudioEnabled(bool enabled) {
    audioCheck_->blockSignals(true);
    audioCheck_->setChecked(enabled);
    audioCheck_->blockSignals(false);
    applyVolume();
}

void LiveStreamTile::setVolumePercent(int percent) {
    const int clamped = qBound(0, percent, 100);
    volumeSlider_->blockSignals(true);
    volumeSlider_->setValue(clamped);
    volumeSlider_->blockSignals(false);
    volumeLabel_->setText(QStringLiteral("%1%").arg(clamped));
    applyVolume();
}

void LiveStreamTile::applyVolume() {
    if (!audioCheck_->isChecked()) {
        audioOutput_->setVolume(0.0f);
        return;
    }
    audioOutput_->setVolume(volumeSlider_->value() / 100.0f);
}

void LiveStreamTile::start() { fetchRoomInfo(); }

void LiveStreamTile::setStreamQuality(int quality) {
    if (streamQuality_ == quality) {
        return;
    }
    streamQuality_ = quality;
    if (!suspended_ && player_->playbackState() != QMediaPlayer::StoppedState) {
        fetchPlayUrl();
    }
}

bool LiveStreamTile::isSuspended() const { return suspended_; }

void LiveStreamTile::suspendPlayback() {
    if (suspended_) {
        return;
    }
    abortPendingRequests();
    player_->stop();
    player_->setSource(QUrl());
    player_->setVideoOutput(nullptr);
    streamProxy_->stop();
    suspended_ = true;
}

void LiveStreamTile::resumePlayback() {
    if (!suspended_) {
        return;
    }
    suspended_ = false;
    player_->setVideoOutput(videoWidget_);
    fetchPlayUrl();
}

void LiveStreamTile::stop() {
    abortPendingRequests();
    player_->stop();
    player_->setSource(QUrl());
    player_->setVideoOutput(nullptr);
    streamProxy_->stop();
    streamUrl_.clear();
    suspended_ = false;
}

void LiveStreamTile::trackReply(QNetworkReply* reply) {
    pendingReplies_.append(reply);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        pendingReplies_.removeAll(reply);
    });
}

void LiveStreamTile::abortPendingRequests() {
    const QList<QNetworkReply*> replies = pendingReplies_;
    pendingReplies_.clear();
    for (QNetworkReply* reply : replies) {
        reply->abort();
        reply->deleteLater();
    }
}

void LiveStreamTile::onPlayerError(QMediaPlayer::Error error, const QString& errorString) {
    Q_UNUSED(error);
    infoLabel_->setText(QStringLiteral("播放错误：%1").arg(errorString));
    emit statusMessage(QStringLiteral("房间 %1 播放错误：%2").arg(roomId_).arg(errorString));
}

void LiveStreamTile::fetchRoomInfo() {
    const QUrl url(
        QStringLiteral("https://api.live.bilibili.com/room/v1/Room/get_info?room_id=%1")
            .arg(roomId_));
    QNetworkReply* reply = network_->get(makeBiliRequest(url));
    trackReply(reply);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            infoLabel_->setText(QStringLiteral("网络错误：%1").arg(reply->errorString()));
            emit statusMessage(QStringLiteral("房间 %1：%2").arg(roomId_).arg(reply->errorString()));
            return;
        }

        const QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();
        if (root.value(QStringLiteral("code")).toInt() != 0) {
            const QString msg = root.value(QStringLiteral("message")).toString();
            infoLabel_->setText(QStringLiteral("API 错误：%1").arg(msg));
            return;
        }

        const QJsonObject data = root.value(QStringLiteral("data")).toObject();
        const int liveStatus = data.value(QStringLiteral("live_status")).toInt();
        const QString title = data.value(QStringLiteral("title")).toString();
        const int online = data.value(QStringLiteral("online")).toInt();
        const QString area = data.value(QStringLiteral("area_name")).toString();

        titleLabel_->setText(title.isEmpty() ? QStringLiteral("房间 %1").arg(roomId_) : title);

        infoLabel_->setText(
            QStringLiteral("%1 · 在线 %2 · %3")
                .arg(area.isEmpty() ? QStringLiteral("—") : area)
                .arg(online)
                .arg(liveStatusText(liveStatus)));

        if (liveStatus != 1) {
            emit statusMessage(QStringLiteral("房间 %1 未开播").arg(roomId_));
            return;
        }

        fetchPlayUrl();
    });
}

void LiveStreamTile::fetchPlayUrl() {
    infoLabel_->setText(QStringLiteral("正在获取直播流…"));
    const QUrl url(
        QStringLiteral("https://api.live.bilibili.com/room/v1/Room/playUrl"
                       "?cid=%1&platform=web&quality=%2&otype=json")
            .arg(roomId_)
            .arg(streamQuality_));
    QNetworkReply* reply = network_->get(makeBiliRequest(url));
    trackReply(reply);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            infoLabel_->setText(QStringLiteral("网络错误：%1").arg(reply->errorString()));
            return;
        }

        const QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();
        if (root.value(QStringLiteral("code")).toInt() != 0) {
            infoLabel_->setText(QStringLiteral("获取播放地址失败"));
            return;
        }

        const QJsonArray durl =
            root.value(QStringLiteral("data")).toObject().value(QStringLiteral("durl")).toArray();
        if (durl.isEmpty()) {
            infoLabel_->setText(QStringLiteral("无可用直播流"));
            return;
        }

        const QString streamUrl = durl.at(0).toObject().value(QStringLiteral("url")).toString();
        if (streamUrl.isEmpty()) {
            infoLabel_->setText(QStringLiteral("流地址为空"));
            return;
        }

        streamUrl_ = streamUrl;
        playStream(streamUrl);
    });
}

void LiveStreamTile::playStream(const QString& streamUrl) {
    if (suspended_) {
        return;
    }
    const QUrl proxyUrl = streamProxy_->start(streamUrl);
    if (proxyUrl.isEmpty()) {
        infoLabel_->setText(QStringLiteral("无法启动本地代理"));
        return;
    }

    player_->setSource(proxyUrl);
    player_->play();
    emit statusMessage(QStringLiteral("房间 %1 正在播放").arg(roomId_));
}

}  // namespace BiliLive
