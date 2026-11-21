#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>

struct MaterialAnimatedTextureFrameCandidate {
    std::int32_t frame;
    bool notifyWrap;
};

// The ordinary AnimatedTexture proxy has a zero start time.
inline std::optional<MaterialAnimatedTextureFrameCandidate> selectMaterialAnimatedTextureFrameCandidate(
    float currentTime, float frameDuration, float rate, std::int32_t frameCount, bool noWrap) {
    if (frameCount <= 0 || !std::isfinite(currentTime) || !std::isfinite(frameDuration) ||
        !std::isfinite(rate) || frameDuration < 0 || rate < 0) {
        return std::nullopt;
    }

    const float current = std::max(currentTime, 0.0f) * rate;
    const float previous = std::max(currentTime - frameDuration, 0.0f) * rate;
    constexpr float int32Limit = 2147483648.0f;
    if (current >= int32Limit || previous >= int32Limit) {
        return std::nullopt;
    }

    const auto currentFrame = static_cast<std::int32_t>(current) % frameCount;
    const auto previousFrame = static_cast<std::int32_t>(previous) % frameCount;
    const bool wrapped = previousFrame > currentFrame;
    if (wrapped && noWrap) {
        return MaterialAnimatedTextureFrameCandidate{
            frameCount - 1, previous < static_cast<float>(frameCount)};
    }
    return MaterialAnimatedTextureFrameCandidate{currentFrame, wrapped};
}
