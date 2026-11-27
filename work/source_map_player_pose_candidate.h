#pragma once

#include <array>
#include <optional>

using SourceMapPlayerVector = std::array<float, 3>;

struct SourceMapPlayerViewPose {
    SourceMapPlayerVector eye, angles;
};

enum class SourceMapPlayerSource { absent, localEdict, mappedClient };

inline SourceMapPlayerSource sourceMapPlayerSource(bool mapped, bool local) {
    if (mapped) return SourceMapPlayerSource::mappedClient;
    return local ? SourceMapPlayerSource::localEdict : SourceMapPlayerSource::absent;
}

inline SourceMapPlayerVector sourceMapPlayerOriginForEye(
    const SourceMapPlayerVector& eye, const SourceMapPlayerVector& viewOffset) {
    SourceMapPlayerVector origin{};
    for (std::size_t axis = 0; axis < origin.size(); ++axis) origin[axis] = eye[axis] - viewOffset[axis];
    return origin;
}

// IFM's neutral-view routine applies to the resolved live client player.
// A failed position query performs neither a snap nor an FOV change.
template<class GetPose, class Snap, class SetFov>
void sourceResetMapPlayerView(GetPose getPose, Snap snap, SetFov setFov) {
    std::optional<SourceMapPlayerViewPose> pose = getPose();
    if (!pose) return;
    pose->angles[2] = 0.f;
    snap(*pose);
    setFov(90);
}
