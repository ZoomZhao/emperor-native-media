#pragma once
#include <cmath>

namespace game_lab {

// 纯组件：将视图局部坐标 (localX, localY) 映射至网格瓦片 (outX, outY)
// 满足：
// 1. 四个 double 输入必须为有限数（isfinite），拒绝 NaN 与无穷 (inf)
// 2. 视宽 viewWidth 与高 viewHeight 必须严格大于 0，mapDim 必须严格大于 0
// 3. 必须先做归一化 (localX / viewWidth)，再乘以 mapDim，避免 DBL_MAX 等有限极值中间乘法溢出变 inf
// 4. 所有拒绝/失败输入均不得修改 outX 与 outY
inline bool mapPointToTile(double localX, double localY, double viewWidth, double viewHeight, int mapDim, int& outX, int& outY) {
    if (!std::isfinite(localX) || !std::isfinite(localY) ||
        !std::isfinite(viewWidth) || !std::isfinite(viewHeight)) {
        return false;
    }
    if (viewWidth <= 0.0 || viewHeight <= 0.0 || mapDim <= 0) {
        return false;
    }
    if (localX < 0.0 || localX >= viewWidth || localY < 0.0 || localY >= viewHeight) {
        return false;
    }

    // 先归一化至 [0.0, 1.0)，防止大有限数乘以 mapDim 发生浮点溢出
    const double normX = localX / viewWidth;
    const double normFromTop = (viewHeight - localY) / viewHeight;
    if (!std::isfinite(normX) || !std::isfinite(normFromTop)) {
        return false;
    }

    // 边界安全 clamp
    double clampedX = normX;
    if (clampedX < 0.0) clampedX = 0.0;
    else if (clampedX >= 1.0) clampedX = 1.0 - 1e-15;

    double clampedY = normFromTop;
    if (clampedY < 0.0) clampedY = 0.0;
    else if (clampedY >= 1.0) clampedY = 1.0 - 1e-15;

    const double scaledX = clampedX * static_cast<double>(mapDim);
    const double scaledY = clampedY * static_cast<double>(mapDim);
    if (!std::isfinite(scaledX) || !std::isfinite(scaledY)) {
        return false;
    }

    int tx = static_cast<int>(std::floor(scaledX));
    int ty = static_cast<int>(std::floor(scaledY));

    if (tx < 0) tx = 0;
    else if (tx >= mapDim) tx = mapDim - 1;

    if (ty < 0) ty = 0;
    else if (ty >= mapDim) ty = mapDim - 1;

    outX = tx;
    outY = ty;
    return true;
}

} // namespace game_lab
