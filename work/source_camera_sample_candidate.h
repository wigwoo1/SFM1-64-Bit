#pragma once

#include "render_camera_candidate.h"
#include <array>
#include <cmath>
#include <optional>
#include <stdexcept>

struct SourceCameraSampleViewCandidate {
    std::array<float, 3> origin;
    float left = 0.f, right = 1.f, bottom = 0.f, top = 1.f;
};

struct SourceCameraLensSampleCandidate {
    std::array<float, 2> point;
    std::array<float, 3> cameraLeft, cameraUp;
    float aperture, focalDistance, referenceFov, aspect;
};

struct SourceCameraRasterSampleCandidate {
    std::array<float, 2> point;
    int width, height;
    float scaleX = 1.f, scaleY = 1.f;
};

// The camera pose and the unadjusted bounds belong to the same shutter time.
// The caller selects lens and raster sample positions independently.
inline SourceCameraSampleViewCandidate sourceCameraSampleViewCandidate(
        SourceCameraSampleViewCandidate view,
        const std::optional<SourceCameraLensSampleCandidate>& lens,
        const std::optional<SourceCameraRasterSampleCandidate>& raster) {
    const auto shiftBounds = [&view](float x, float y) {
        view.left += x;
        view.right += x;
        view.bottom += y;
        view.top += y;
    };
    if (lens) {
        if (!std::isfinite(lens->referenceFov) || lens->referenceFov <= 0.f ||
            lens->referenceFov >= 180.f || !std::isfinite(lens->aspect) || lens->aspect <= 0.f)
            throw std::invalid_argument("invalid camera lens sampling projection");
        const auto normalize = [](std::array<float, 3> axis) {
            const float length = std::sqrt((axis[0] * axis[0] + axis[1] * axis[1]) + axis[2] * axis[2]);
            const float inverseLength = 1.f / (length + 0x1p-23f);
            for (auto& component : axis) component *= inverseLength;
            return axis;
        };
        const auto left = normalize(lens->cameraLeft);
        const auto up = normalize(lens->cameraUp);
        const float x = (lens->point[0] * lens->aperture) * .0008f;
        const float y = (lens->point[1] * lens->aperture) * .0008f;
        const double halfAngle = double(lens->referenceFov) * .5 * double(0.01745329238474369f);
        const float halfTangent = float(std::tan(halfAngle) * (double(lens->aspect) * .75));
        const float focusWidth = float(double(lens->focalDistance) * double(halfTangent) * 2.);
        const float lensX = x * focusWidth;
        const float lensY = y * focusWidth;
        const float inverseAspect = 1.f / lens->aspect;
        shiftBounds(x, y);
        for (size_t axis = 0; axis < view.origin.size(); ++axis) {
            const float offset = left[axis] * lensX - (up[axis] * lensY) * inverseAspect;
            view.origin[axis] += offset;
        }
    }
    if (raster) {
        if (raster->width <= 0 || raster->height <= 0)
            throw std::invalid_argument("invalid camera raster sampling dimensions");
        const float x = ((raster->scaleX * .6f) / float(raster->width)) * raster->point[0];
        const float y = ((raster->scaleY * .6f) / float(raster->height)) * raster->point[1];
        shiftBounds(x, y);
    }
    for (float component : view.origin)
        if (!std::isfinite(component)) throw std::runtime_error("camera sample origin overflow");
    if (!std::isfinite(view.left) || !std::isfinite(view.right) ||
        !std::isfinite(view.bottom) || !std::isfinite(view.top))
        throw std::runtime_error("camera sample bounds overflow");
    return view;
}

// The bounds are fractions of the centered frustum at the near plane.
// Source projects with -Z forward; Vulkan's framebuffer has Y down.
inline RenderCameraCandidate sourceCameraSampleProjectionCandidate(
        float horizontalFov, float aspect, float nearPlane, float farPlane,
        const SourceCameraSampleViewCandidate& view) {
    if (!std::isfinite(horizontalFov) || horizontalFov <= 0.f || horizontalFov >= 180.f ||
        !std::isfinite(aspect) || aspect <= 0.f || !std::isfinite(nearPlane) ||
        !std::isfinite(farPlane) || nearPlane <= 0.f || farPlane <= nearPlane ||
        view.right <= view.left || view.top <= view.bottom)
        throw std::invalid_argument("invalid off-center camera frustum");
    const float halfAngle = float((double(horizontalFov) * 3.141592653589793) / 360.);
    const double twiceNear = 2. * nearPlane;
    const float fullWidth = float(double(std::tan(halfAngle)) * twiceNear);
    const float halfWidth = fullWidth * .5f;
    const float halfHeight = float(double(fullWidth) / aspect) * .5f;
    const auto edge = [](float halfExtent, float bound) {
        return float(double(halfExtent) * bound - double(halfExtent) * (1. - double(bound)));
    };
    const float left = edge(halfWidth, view.left), right = edge(halfWidth, view.right);
    const float bottom = edge(halfHeight, view.bottom), top = edge(halfHeight, view.top);
    const float width = right - left, height = top - bottom;
    const double depth = double(nearPlane) - farPlane;
    RenderCameraCandidate result{{
        float(twiceNear / width), 0.f, (right + left) / width, 0.f,
        0.f, -float(twiceNear / height), -(top + bottom) / height, 0.f,
        0.f, 0.f, float(farPlane / depth), float(double(nearPlane) * farPlane / depth),
        0.f, 0.f, -1.f, 0.f
    }};
    for (float component : result.rows)
        if (!std::isfinite(component)) throw std::runtime_error("off-center camera projection overflow");
    return result;
}
