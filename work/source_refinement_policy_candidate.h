#pragma once

#include "time_scale.h"
#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>

struct SourceRefinementSettingsCandidate {
    bool enabled=false, motionBlur=false, depthOfField=false, antialiasing=false;
    std::optional<int32_t> motionQualityOverride, depthQualityOverride;
    bool overrideShutter=false;
    std::optional<int32_t> shutterOverrideTicks;
};

struct SourceRefinementCameraCandidate {
    int32_t motionQuality=0, depthQuality=0, shutterTicks=0;
};

struct SourceRefinementPlanCandidate {
    uint32_t shutterGroups=1, lensSamplesPerGroup=1, totalSamples=1;
    int32_t shutterTicks=0;
};

inline int32_t sourceShutterSecondsToTicksCandidate(float seconds) {
    const float roundedInput=seconds*10000.f+.5f;
    if(!std::isfinite(roundedInput)||roundedInput < -2147483648.f||roundedInput >= 2147483648.f)
        throw std::runtime_error("Source shutter seconds outside DmTime range");
    return int32_t(std::floor(roundedInput));
}

inline SourceRefinementPlanCandidate sourceRefinementPlanCandidate(
        const SourceRefinementSettingsCandidate& settings,
        const std::optional<SourceRefinementCameraCandidate>& camera,uint32_t packedFrameRate) {
    const auto frames=packedFrameRate&65535u;
    const auto ticks=packedFrameRate>>16;
    if(!frames||!ticks)throw std::invalid_argument("invalid Source refinement frame rate");
    SourceRefinementPlanCandidate result;
    const auto halfFrame=scaleTicks(int32_t(ticks/frames),.5f);
    if(settings.overrideShutter)result.shutterTicks=settings.shutterOverrideTicks.value_or(halfFrame);
    else result.shutterTicks=camera?camera->shutterTicks:halfFrame;

    if(!settings.enabled||!camera)return result;
    if(settings.motionBlur){
        const auto quality=std::clamp(settings.motionQualityOverride.value_or(camera->motionQuality),0,5);
        result.shutterGroups=8u<<quality;
    }
    result.totalSamples=result.shutterGroups;
    if(settings.depthOfField){
        const auto quality=std::clamp(settings.depthQualityOverride.value_or(camera->depthQuality),0,7);
        result.totalSamples=std::max(result.totalSamples,8u<<quality);
    }
    if(settings.antialiasing)result.totalSamples=std::max(result.totalSamples,4u);
    result.lensSamplesPerGroup=result.totalSamples/result.shutterGroups;
    return result;
}
