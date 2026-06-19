#pragma once

#include <QString>

namespace BiliLive {

struct GridShape {
    int cols = 0;
    int rows = 0;
};

GridShape gridShapeForCount(int count);
QString gridLayoutName(int count, const GridShape& shape);

}  // namespace BiliLive
