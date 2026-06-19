#include "core/grid_layout.h"

#include <cmath>

namespace BiliLive {

GridShape gridShapeForCount(int count) {
    if (count <= 0) {
        return {};
    }
    const int cols = static_cast<int>(std::ceil(std::sqrt(static_cast<double>(count))));
    const int rows = (count + cols - 1) / cols;
    return {cols, rows};
}

QString gridLayoutName(int count, const GridShape& shape) {
    if (count <= 1) {
        return QStringLiteral("单画面");
    }
    if (count == 4 && shape.cols == 2 && shape.rows == 2) {
        return QStringLiteral("2×2 田字格");
    }
    if (count <= 9 && shape.cols == 3 && shape.rows == 3) {
        return QStringLiteral("%1×%2 九宫格").arg(shape.cols).arg(shape.rows);
    }
    return QStringLiteral("%1×%2 网格").arg(shape.cols).arg(shape.rows);
}

}  // namespace BiliLive
