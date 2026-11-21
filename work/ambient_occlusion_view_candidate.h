#pragma once
#include "render_camera_candidate.h"
#include <array>
#include <cstddef>
#include <cstdint>

// std140 layout of OcclusionView in sfm64-ambient-occlusion-mask-candidate.hlsl.
struct AmbientOcclusionViewCandidate {
    using Row = std::array<float, 4>;
    Row topLeftRay{}, horizontalRaySpan{}, verticalRaySpan{}, eyeAndBias{}, eyeForward{};
    Row clipX{}, clipY{}, clipW{}, noiseTransform{}, centerRadiusFalloff{};
    std::array<Row, 16> sampleDirections{};
};
static_assert(sizeof(AmbientOcclusionViewCandidate) == 416);
static_assert(offsetof(AmbientOcclusionViewCandidate, eyeAndBias) == 48);
static_assert(offsetof(AmbientOcclusionViewCandidate, sampleDirections) == 160);

// Authored noise for the original diagnostic mode.
inline constexpr uint32_t ambientOcclusionNoiseExtentCandidate = 4;
inline std::array<AmbientOcclusionViewCandidate::Row, 16> ambientOcclusionNoiseCandidate() {
    std::array<AmbientOcclusionViewCandidate::Row, 16> pixels{};
    constexpr double goldenAngle = 2.399963229728653;
    for (size_t i = 0; i < pixels.size(); ++i) {
        const double z = 1 - (static_cast<double>((i * 7) % pixels.size()) + .5) / 8;
        const double angle = static_cast<double>(i) * goldenAngle;
        const double r = std::sqrt(1 - z*z);
        pixels[i] = {static_cast<float>(.5 + .5*r*std::cos(angle)),
                     static_cast<float>(.5 + .5*r*std::sin(angle)),
                     static_cast<float>(.5 + .5*z), 1};
    }
    return pixels;
}

inline AmbientOcclusionViewCandidate prepareAmbientOcclusionViewCandidate(
        const RenderCameraCandidate& camera, float bias = .001f,
        float radius = 48.f, float inverseFalloff = 1.f/24.f,
        uint32_t noiseExtent = ambientOcclusionNoiseExtentCandidate) {
    using Vec = std::array<double, 3>;
    const auto& r = camera.rows;
    const Vec x{r[0],r[1],r[2]}, y{r[4],r[5],r[6]}, forward{r[12],r[13],r[14]};
    const auto cross = [](const Vec& a, const Vec& b) -> Vec {
        return {a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]};
    };
    const auto dot = [](const Vec& a, const Vec& b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; };
    const Vec inverseX = cross(y, forward), inverseY = cross(forward, x), inverseW = cross(x, y);
    const double determinant = dot(x, inverseX);
    if (!std::isfinite(determinant) || std::abs(determinant) < 1e-8 ||
        std::abs(dot(forward, forward)-1) > .001)
        throw std::runtime_error("AO projection must use an invertible perspective camera with linear clip W");
    const auto resolve = [&](double px, double py, double depth) -> AmbientOcclusionViewCandidate::Row {
        AmbientOcclusionViewCandidate::Row result{};
        for (size_t axis = 0; axis < 3; ++axis) {
            result[axis] = static_cast<float>((inverseX[axis]*px+inverseY[axis]*py+inverseW[axis]*depth)/determinant);
            if (!std::isfinite(result[axis])) throw std::runtime_error("AO camera parameters overflow");
        }
        return result;
    };
    AmbientOcclusionViewCandidate view;
    view.topLeftRay = resolve(-1, -1, 1);
    view.horizontalRaySpan = resolve(2, 0, 0);
    view.verticalRaySpan = resolve(0, 2, 0);
    view.eyeAndBias = resolve(-r[3], -r[7], -r[15]);
    view.eyeAndBias[3] = bias;
    view.eyeForward = {r[12],r[13],r[14],0};
    view.clipX = {r[0],r[1],r[2],r[3]};
    // The shader accepts Y-up clip rows, whereas the viewport camera is Y-down.
    view.clipY = {-r[4],-r[5],-r[6],-r[7]};
    view.clipW = {r[12],r[13],r[14],r[15]};
    if (!noiseExtent) throw std::runtime_error("AO noise extent must be positive");
    const float tile = float(noiseExtent);
    // Choose the center of each point-sampled texel at zero jitter.
    view.noiseTransform = {1/tile,1/tile,.5f/tile,.5f/tile};
    view.centerRadiusFalloff = {.5f,.5f,radius,inverseFalloff};
    constexpr double goldenAngle = 2.399963229728653;
    for (size_t i = 0; i < view.sampleDirections.size(); ++i) {
        const double h = (static_cast<double>(i)+.5)/16;
        const double z = 1-2*h, angle = static_cast<double>(i)*goldenAngle;
        const double sampleScale = .4+.6*h;
        const double xy = sampleScale*std::sqrt(1-z*z);
        view.sampleDirections[i] = {static_cast<float>(xy*std::cos(angle)),static_cast<float>(xy*std::sin(angle)),
                                    static_cast<float>(z*sampleScale),0};
    }
    return view;
}
