#include "core/app_settings.h"

#include <QCoreApplication>

namespace BiliLive {

QString configFilePath() {
    return QCoreApplication::applicationDirPath() + QStringLiteral("/qt_bilibili_live.ini");
}

QSettings makeAppSettings() {
    return QSettings(configFilePath(), QSettings::IniFormat);
}

bool defaultGpuAccelerationEnabled() {
#if defined(Q_OS_WIN)
    return true;
#else
    return false;
#endif
}

void applyFfmpegGpuEnvironment(bool enabled) {
    if (enabled) {
#if defined(Q_OS_WIN)
        qputenv("QT_FFMPEG_DECODING_HW_DEVICE_TYPES", "d3d12va,d3d11va,dxva2");
#elif defined(Q_OS_LINUX)
        qputenv("QT_FFMPEG_DECODING_HW_DEVICE_TYPES", "vaapi,vulkan");
#elif defined(Q_OS_MACOS)
        qputenv("QT_FFMPEG_DECODING_HW_DEVICE_TYPES", "videotoolbox");
#endif
        qputenv("QT_FFMPEG_HW_ALLOW_PROFILE_MISMATCH", "1");
        qunsetenv("QT_DISABLE_HW_TEXTURES_CONVERSION");
    } else {
        qputenv("QT_FFMPEG_DECODING_HW_DEVICE_TYPES", ",");
        qputenv("QT_DISABLE_HW_TEXTURES_CONVERSION", "1");
    }
}

}  // namespace BiliLive
