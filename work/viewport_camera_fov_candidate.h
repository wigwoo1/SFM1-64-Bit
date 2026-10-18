#pragma once
#include <cmath>
#include <stdexcept>

// Installed ifm.dll viewport path 0x10292028..0x1029208C converts the
// stored 4:3 horizontal angle before writing CViewSetup::fov. This is
// separate from CDmeCamera::GetProjectionMatrix's raw horizontal angle.
inline float viewportCameraHorizontalFovCandidate(float referenceFov, float aspect) {
    if (!std::isfinite(referenceFov) || referenceFov <= 0 || referenceFov >= 180 ||
        !std::isfinite(aspect) || aspect <= 0)
        throw std::runtime_error("viewport camera FOV parameters");
    const float radians = referenceFov * 0.01745329238474369f;
    const float widthRatio = aspect * 0.75f;
    const float halfAngle = radians * 0.5f;
    const float tangent = std::tan(halfAngle);
    const float widened = tangent * widthRatio;
    const float halfResult = std::atan(widened);
    const float result = (halfResult * 2.0f) * 57.295780181884766f;
    if (!std::isfinite(result) || result <= 0 || result >= 180)
        throw std::runtime_error("viewport camera FOV overflow");
    return result;
}
