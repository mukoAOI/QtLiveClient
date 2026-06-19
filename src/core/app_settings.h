#pragma once

#include <QSettings>
#include <QString>

namespace BiliLive {

QString configFilePath();
QSettings makeAppSettings();
bool defaultGpuAccelerationEnabled();
void applyFfmpegGpuEnvironment(bool enabled);

}  // namespace BiliLive
