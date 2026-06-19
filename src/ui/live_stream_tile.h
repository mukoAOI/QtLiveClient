#pragma once

#include <QFrame>
#include <QMediaPlayer>

class QAudioOutput;
class QCheckBox;
class QLabel;
class QNetworkAccessManager;
class QNetworkReply;
class QNetworkReply;
class QPushButton;
class QSlider;
class QVideoWidget;

namespace BiliLive {

class LocalStreamProxy;

class LiveStreamTile : public QFrame {
    Q_OBJECT

public:
    LiveStreamTile(qint64 roomId, const QString& inputUrl, QNetworkAccessManager* network,
                   QWidget* parent = nullptr);

    qint64 roomId() const;
    QString inputUrl() const;
    bool isAudioEnabled() const;
    int volumePercent() const;

    void setAudioEnabled(bool enabled);
    void setVolumePercent(int percent);
    void applyVolume();

    void start();
    void setStreamQuality(int quality);
    bool isSuspended() const;
    void suspendPlayback();
    void resumePlayback();
    void stop();

signals:
    void closeRequested(LiveStreamTile* self);
    void statusMessage(const QString& message);
    void exclusiveToggleRequested(LiveStreamTile* self);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    void trackReply(QNetworkReply* reply);
    void abortPendingRequests();
    void onPlayerError(QMediaPlayer::Error error, const QString& errorString);
    void fetchRoomInfo();
    void fetchPlayUrl();
    void playStream(const QString& streamUrl);

    qint64 roomId_ = 0;
    int streamQuality_ = 0;
    bool suspended_ = false;
    QString inputUrl_;
    QString streamUrl_;
    QList<QNetworkReply*> pendingReplies_;
    QLabel* titleLabel_ = nullptr;
    QLabel* infoLabel_ = nullptr;
    QCheckBox* audioCheck_ = nullptr;
    QSlider* volumeSlider_ = nullptr;
    QLabel* volumeLabel_ = nullptr;
    QPushButton* closeBtn_ = nullptr;
    QVideoWidget* videoWidget_ = nullptr;

    QNetworkAccessManager* network_ = nullptr;
    LocalStreamProxy* streamProxy_ = nullptr;
    QMediaPlayer* player_ = nullptr;
    QAudioOutput* audioOutput_ = nullptr;
};

}  // namespace BiliLive
