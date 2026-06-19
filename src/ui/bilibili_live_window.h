#pragma once

#include <QMainWindow>

class QCheckBox;
class QLabel;
class QLineEdit;
class QNetworkAccessManager;
class QPushButton;
class QSlider;

namespace BiliLive {

class LiveStreamTile;

class BilibiliLiveWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit BilibiliLiveWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void addStream();

private:
    int gridStreamQuality() const;
    void applyGridStreamQuality();
    void onGlobalVolumeChanged(int value);
    void onGpuAccelToggled(bool enabled);
    void saveSession();
    void loadSession();
    bool addStreamFromInput(const QString& input, bool showWarnings, bool deferLayout = false);
    bool hasRoom(qint64 roomId) const;
    void removeTile(LiveStreamTile* tile);
    void toggleExclusiveMode(LiveStreamTile* tile);
    void enterExclusiveMode(LiveStreamTile* tile);
    void exitExclusiveMode();
    void rebuildGrid();

    QLineEdit* urlEdit_ = nullptr;
    QPushButton* addBtn_ = nullptr;
    QSlider* globalVolumeSlider_ = nullptr;
    QLabel* globalVolumeLabel_ = nullptr;
    QCheckBox* gpuAccelCheck_ = nullptr;
    QLabel* hintLabel_ = nullptr;
    QWidget* gridArea_ = nullptr;
    QWidget* gridHost_ = nullptr;
    QList<LiveStreamTile*> tiles_;
    LiveStreamTile* exclusiveTile_ = nullptr;
    bool exclusiveBusy_ = false;
    QNetworkAccessManager* network_ = nullptr;
};

}  // namespace BiliLive
