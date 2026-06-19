#include "ui/bilibili_live_window.h"

#include "core/app_settings.h"

#include <QApplication>
#include <QSettings>

int main(int argc, char* argv[]) {
    qputenv("QT_MEDIA_BACKEND", "ffmpeg");
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("bench"));
    QApplication::setApplicationName(QStringLiteral("qt_bilibili_live"));

    QSettings bootSettings(BiliLive::configFilePath(), QSettings::IniFormat);
    const bool gpuEnabled =
        bootSettings.value(QStringLiteral("gpuAcceleration"),
                           BiliLive::defaultGpuAccelerationEnabled())
            .toBool();
    BiliLive::applyFfmpegGpuEnvironment(gpuEnabled);

    BiliLive::BilibiliLiveWindow window;
    window.show();

    return app.exec();
}
