#include "ui/bilibili_live_window.h"

#include "core/app_settings.h"
#include "core/bilibili_utils.h"
#include "core/constants.h"
#include "core/grid_layout.h"
#include "ui/live_stream_tile.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QPushButton>
#include <QSettings>
#include <QSlider>
#include <QSplitter>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>

namespace BiliLive {

BilibiliLiveWindow::BilibiliLiveWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("Bilibili 多路直播"));
    resize(1280, 800);

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);

    auto* topBar = new QHBoxLayout();
    topBar->addWidget(new QLabel(QStringLiteral("直播间链接："), central));
    urlEdit_ = new QLineEdit(central);
    urlEdit_->setPlaceholderText(QStringLiteral("https://live.bilibili.com/房间号"));
    topBar->addWidget(urlEdit_, 1);

    addBtn_ = new QPushButton(QStringLiteral("添加"), central);
    topBar->addWidget(addBtn_);

    topBar->addSpacing(12);
    topBar->addWidget(new QLabel(QStringLiteral("全局音量："), central));
    globalVolumeSlider_ = new QSlider(Qt::Horizontal, central);
    globalVolumeSlider_->setRange(0, 100);
    globalVolumeSlider_->setValue(100);
    globalVolumeSlider_->setFixedWidth(120);
    globalVolumeSlider_->setToolTip(QStringLiteral("同步调节所有直播的音量"));
    topBar->addWidget(globalVolumeSlider_);
    globalVolumeLabel_ = new QLabel(QStringLiteral("100%"), central);
    globalVolumeLabel_->setFixedWidth(40);
    globalVolumeLabel_->setStyleSheet(QStringLiteral("color: #666;"));
    topBar->addWidget(globalVolumeLabel_);

    gpuAccelCheck_ = new QCheckBox(QStringLiteral("GPU 加速"), central);
    gpuAccelCheck_->setToolTip(
        QStringLiteral("启用 FFmpeg 硬件解码与 GPU 纹理渲染（D3D11/D3D12）。修改后需重启程序生效。"));
    topBar->addWidget(gpuAccelCheck_);

    layout->addLayout(topBar);

    hintLabel_ = new QLabel(
        QStringLiteral("可添加多个直播源，系统按数量自动排成田字格/九宫格。"
                       "拖动分隔条调节大小；双击某路直播进入独占模式，再次双击退出。"),
        central);
    hintLabel_->setWordWrap(true);
    hintLabel_->setStyleSheet(QStringLiteral("color: #666;"));
    layout->addWidget(hintLabel_);

    gridArea_ = new QWidget(central);
    auto* gridAreaLayout = new QVBoxLayout(gridArea_);
    gridAreaLayout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(gridArea_, 1);

    setCentralWidget(central);
    statusBar()->showMessage(QStringLiteral("就绪 — 输入链接后点击「添加」"));

    network_ = new QNetworkAccessManager(this);

    connect(addBtn_, &QPushButton::clicked, this, &BilibiliLiveWindow::addStream);
    connect(urlEdit_, &QLineEdit::returnPressed, this, &BilibiliLiveWindow::addStream);
    connect(globalVolumeSlider_, &QSlider::valueChanged, this,
            &BilibiliLiveWindow::onGlobalVolumeChanged);
    connect(gpuAccelCheck_, &QCheckBox::toggled, this, &BilibiliLiveWindow::onGpuAccelToggled);

    const bool gpuEnabled =
        makeAppSettings()
            .value(QStringLiteral("gpuAcceleration"), defaultGpuAccelerationEnabled())
            .toBool();
    gpuAccelCheck_->blockSignals(true);
    gpuAccelCheck_->setChecked(gpuEnabled);
    gpuAccelCheck_->blockSignals(false);

    QTimer::singleShot(0, this, [this] { loadSession(); });
}

void BilibiliLiveWindow::closeEvent(QCloseEvent* event) {
    for (LiveStreamTile* tile : tiles_) {
        tile->stop();
    }
    saveSession();
    QMainWindow::closeEvent(event);
}

int BilibiliLiveWindow::gridStreamQuality() const {
    return tiles_.size() > 1 ? kQualityGrid : kQualityOrigin;
}

void BilibiliLiveWindow::applyGridStreamQuality() {
    const int quality = gridStreamQuality();
    for (LiveStreamTile* tile : tiles_) {
        tile->setStreamQuality(quality);
    }
}

void BilibiliLiveWindow::onGlobalVolumeChanged(int value) {
    globalVolumeLabel_->setText(QStringLiteral("%1%").arg(value));
    for (LiveStreamTile* tile : tiles_) {
        tile->setVolumePercent(value);
    }
}

void BilibiliLiveWindow::onGpuAccelToggled(bool enabled) {
    QSettings settings = makeAppSettings();
    settings.setValue(QStringLiteral("gpuAcceleration"), enabled);
    settings.sync();
    statusBar()->showMessage(
        enabled ? QStringLiteral("GPU 加速已开启，请重启程序后生效")
                : QStringLiteral("GPU 加速已关闭，请重启程序后生效"));
}

void BilibiliLiveWindow::saveSession() {
    QSettings settings = makeAppSettings();
    settings.setValue(QStringLiteral("geometry"), saveGeometry());

    QStringList streams;
    QVariantMap volumes;
    for (const LiveStreamTile* tile : tiles_) {
        const QString roomKey = QString::number(tile->roomId());
        streams.append(normalizeLiveUrl(tile->roomId()));
        volumes.insert(roomKey, tile->volumePercent());
    }
    settings.setValue(QStringLiteral("streams"), streams);
    settings.setValue(QStringLiteral("volumes"), volumes);
    settings.setValue(QStringLiteral("globalVolume"), globalVolumeSlider_->value());
    settings.setValue(QStringLiteral("gpuAcceleration"), gpuAccelCheck_->isChecked());
    settings.sync();
}

void BilibiliLiveWindow::loadSession() {
    QSettings settings = makeAppSettings();
    const QByteArray geometry = settings.value(QStringLiteral("geometry")).toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }

    const int globalVolume = settings.value(QStringLiteral("globalVolume"), 100).toInt();
    globalVolumeSlider_->blockSignals(true);
    globalVolumeSlider_->setValue(qBound(0, globalVolume, 100));
    globalVolumeSlider_->blockSignals(false);
    globalVolumeLabel_->setText(QStringLiteral("%1%").arg(globalVolumeSlider_->value()));

    const QStringList streams = settings.value(QStringLiteral("streams")).toStringList();
    if (streams.isEmpty()) {
        return;
    }

    for (const QString& url : streams) {
        addStreamFromInput(url, false, true);
    }

    rebuildGrid();
    applyGridStreamQuality();

    const QVariantMap volumes = settings.value(QStringLiteral("volumes")).toMap();
    for (LiveStreamTile* tile : tiles_) {
        const QString roomKey = QString::number(tile->roomId());
        const int volume = volumes.value(roomKey, 0).toInt();
        tile->setVolumePercent(volume);
        tile->setAudioEnabled(volume > 0);
    }
    if (volumes.isEmpty() && !tiles_.isEmpty()) {
        tiles_.first()->setVolumePercent(80);
        tiles_.first()->setAudioEnabled(true);
    }

    for (LiveStreamTile* tile : tiles_) {
        tile->start();
    }

    if (!tiles_.isEmpty()) {
        const GridShape shape = gridShapeForCount(tiles_.size());
        statusBar()->showMessage(
            QStringLiteral("已恢复 %1 路直播 · %2 · 配置：%3")
                .arg(tiles_.size())
                .arg(gridLayoutName(tiles_.size(), shape))
                .arg(configFilePath()));
    }
}

bool BilibiliLiveWindow::addStreamFromInput(const QString& input, bool showWarnings,
                                            bool deferLayout) {
    const QString trimmed = input.trimmed();
    const qint64 roomId = parseRoomId(trimmed);
    if (roomId <= 0) {
        if (showWarnings) {
            QMessageBox::warning(this, QStringLiteral("链接无效"),
                                 QStringLiteral("请输入有效的 Bilibili 直播间链接或房间号。"));
        }
        return false;
    }

    if (hasRoom(roomId)) {
        if (showWarnings) {
            QMessageBox::information(this, QStringLiteral("重复链接"),
                                     QStringLiteral("房间 %1 已在窗口中，请勿重复添加。")
                                         .arg(roomId));
        }
        return false;
    }

    const QString savedUrl = normalizeLiveUrl(roomId);
    auto* tile = new LiveStreamTile(roomId, savedUrl, network_, gridArea_);
    tile->setStreamQuality(deferLayout ? kQualityGrid : gridStreamQuality());
    tile->hide();
    connect(tile, &LiveStreamTile::closeRequested, this, &BilibiliLiveWindow::removeTile);
    connect(tile, &LiveStreamTile::exclusiveToggleRequested, this,
            &BilibiliLiveWindow::toggleExclusiveMode);
    connect(tile, &LiveStreamTile::statusMessage, this,
            [this](const QString& msg) { statusBar()->showMessage(msg); });

    tiles_.append(tile);

    if (!deferLayout) {
        rebuildGrid();
        applyGridStreamQuality();

        if (tiles_.size() == 1) {
            tile->setVolumePercent(80);
            tile->setAudioEnabled(true);
        } else {
            tile->setVolumePercent(globalVolumeSlider_->value());
        }

        if (showWarnings) {
            const GridShape shape = gridShapeForCount(tiles_.size());
            statusBar()->showMessage(
                QStringLiteral("已添加房间 %1，共 %2 路 · %3")
                    .arg(roomId)
                    .arg(tiles_.size())
                    .arg(gridLayoutName(tiles_.size(), shape)));
        }

        tile->start();
    }

    return true;
}

bool BilibiliLiveWindow::hasRoom(qint64 roomId) const {
    for (const LiveStreamTile* tile : tiles_) {
        if (tile->roomId() == roomId) {
            return true;
        }
    }
    return false;
}

void BilibiliLiveWindow::removeTile(LiveStreamTile* tile) {
    if (exclusiveTile_ == tile) {
        exclusiveTile_ = nullptr;
    }
    tile->stop();
    tiles_.removeAll(tile);
    tile->deleteLater();
    rebuildGrid();
    statusBar()->showMessage(QStringLiteral("已移除直播源，当前 %1 路").arg(tiles_.size()));
}

void BilibiliLiveWindow::toggleExclusiveMode(LiveStreamTile* tile) {
    if (tiles_.size() <= 1 || exclusiveBusy_) {
        return;
    }
    exclusiveBusy_ = true;
    if (exclusiveTile_ == tile) {
        exitExclusiveMode();
    } else {
        enterExclusiveMode(tile);
    }
    QTimer::singleShot(300, this, [this]() { exclusiveBusy_ = false; });
}

void BilibiliLiveWindow::enterExclusiveMode(LiveStreamTile* tile) {
    exclusiveTile_ = tile;

    for (LiveStreamTile* t : tiles_) {
        if (t != tile) {
            t->suspendPlayback();
            t->hide();
        }
    }

    tile->setStreamQuality(kQualityOrigin);
    statusBar()->showMessage(QStringLiteral("独占模式：%1 · 双击画面退出").arg(tile->roomId()));
}

void BilibiliLiveWindow::exitExclusiveMode() {
    if (!exclusiveTile_) {
        return;
    }
    exclusiveTile_ = nullptr;

    for (LiveStreamTile* t : tiles_) {
        if (!t->isSuspended()) {
            t->suspendPlayback();
        }
        t->show();
    }

    applyGridStreamQuality();

    int delayMs = 100;
    for (LiveStreamTile* t : tiles_) {
        QPointer<LiveStreamTile> guard(t);
        QTimer::singleShot(delayMs, t, [guard]() {
            if (guard) {
                guard->resumePlayback();
            }
        });
        delayMs += 200;
    }

    if (!tiles_.isEmpty()) {
        const GridShape shape = gridShapeForCount(tiles_.size());
        statusBar()->showMessage(
            QStringLiteral("当前 %1 路 · %2")
                .arg(tiles_.size())
                .arg(gridLayoutName(tiles_.size(), shape)));
    }
}

void BilibiliLiveWindow::rebuildGrid() {
    exclusiveTile_ = nullptr;

    QWidget* oldHost = gridHost_;
    gridHost_ = nullptr;

    const int count = tiles_.size();
    if (count == 0) {
        if (oldHost) {
            qobject_cast<QVBoxLayout*>(gridArea_->layout())->removeWidget(oldHost);
            oldHost->hide();
            oldHost->deleteLater();
        }
        return;
    }

    const GridShape shape = gridShapeForCount(count);
    gridHost_ = new QWidget(gridArea_);
    auto* hostLayout = new QVBoxLayout(gridHost_);
    hostLayout->setContentsMargins(0, 0, 0, 0);
    hostLayout->setSpacing(0);

    auto* vSplitter = new QSplitter(Qt::Vertical, gridHost_);
    vSplitter->setChildrenCollapsible(false);
    vSplitter->setHandleWidth(6);
    hostLayout->addWidget(vSplitter);

    int index = 0;
    for (int row = 0; row < shape.rows; ++row) {
        auto* hSplitter = new QSplitter(Qt::Horizontal, vSplitter);
        hSplitter->setChildrenCollapsible(false);
        hSplitter->setHandleWidth(6);

        for (int col = 0; col < shape.cols && index < count; ++col, ++index) {
            LiveStreamTile* tile = tiles_[index];
            hSplitter->addWidget(tile);
            tile->show();
            hSplitter->setStretchFactor(col, 1);
        }
        vSplitter->addWidget(hSplitter);
        vSplitter->setStretchFactor(row, 1);
    }

    qobject_cast<QVBoxLayout*>(gridArea_->layout())->addWidget(gridHost_, 1);

    if (oldHost) {
        qobject_cast<QVBoxLayout*>(gridArea_->layout())->removeWidget(oldHost);
        oldHost->hide();
        oldHost->deleteLater();
    }

    const QString layoutName = gridLayoutName(count, shape);
    statusBar()->showMessage(QStringLiteral("当前 %1 路 · %2").arg(count).arg(layoutName));
}

void BilibiliLiveWindow::addStream() {
    const QString input = urlEdit_->text().trimmed();
    if (addStreamFromInput(input, true)) {
        urlEdit_->clear();
    }
}

}  // namespace BiliLive
