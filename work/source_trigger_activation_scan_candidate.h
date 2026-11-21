#pragma once

#include "source_map_trigger_actor_candidate.h"
#include "source_solid_trigger_scan_candidate.h"

inline SourceMapTouchScanCandidate sourcePostClientActiveScanCandidate(
    bool disabled, bool hasEdict, bool isWorld, uint8_t solidType, uint16_t solidFlags, bool hasBrushModel) {
    if (disabled) return SourceMapTouchScanCandidate::none;
    return sourceMapTouchScanCandidate(hasEdict, isWorld, solidType, solidFlags, hasBrushModel);
}

struct SourceTriggerSolidCandidate {
    uint32_t identity;
    bool isStaticProp, isTrigger, acceptsTriggerFlags, isServerEntity, hasNetworkable;
    SourceTriggerPointCandidate origin;
    uint32_t solidType;
    SourceTriggerBoundsCandidate local, surrounding;
};

// TriggerMoved enumerates the solid partition using the trigger's bounds.
// Each admitted solid supplies a stationary hull for clipping against the trigger.
template<typename Clip>
std::vector<uint32_t> sourceTriggerMovedContactsCandidate(
    uint32_t triggerFlags, bool useLocalBoxBounds, const SourceTriggerBoundsCandidate& triggerBounds,
    const std::vector<SourceTriggerSolidCandidate>& candidates, Clip clip) {
    constexpr uint32_t useTriggerBounds = 0x80;
    constexpr uint32_t solidTraceMask = 0x0200400b;
    std::vector<uint32_t> contacts;
    for (const auto& candidate : candidates) {
        if (candidate.isStaticProp || candidate.isTrigger || !candidate.acceptsTriggerFlags ||
            !candidate.isServerEntity || !candidate.hasNetworkable) continue;
        const auto hull = sourceSolidTriggerSweepCandidate(candidate.origin, std::nullopt,
            candidate.solidType, useLocalBoxBounds, candidate.local, candidate.surrounding);
        const bool hit = triggerFlags & useTriggerBounds
            ? sourceSolidTriggerBoundsHitCandidate(hull, triggerBounds)
            : (clip(hull, candidate.identity, solidTraceMask) & solidTraceMask) != 0;
        if (hit) contacts.push_back(candidate.identity);
    }
    return contacts;
}
