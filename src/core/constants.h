#pragma once

namespace BiliLive {

constexpr auto kReferer = "https://live.bilibili.com/";
constexpr auto kUserAgent =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36";

constexpr int kProxyChunkBytes = 64 * 1024;
constexpr int kProxyMaxClientBuffer = 256 * 1024;
constexpr int kQualityOrigin = 4;
constexpr int kQualityGrid = 2;

}  // namespace BiliLive
