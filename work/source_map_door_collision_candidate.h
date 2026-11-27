#pragma once

#include "source_bsp_map_doors_candidate.h"
#include "source_translated_brush_box_trace_candidate.h"

// The caller owns collision data and determines spatial query order and when
// to finish scans. A bound brush supplies the geometry of one map trigger.
class SourceMapDoorBrushCandidate {
    SourceTriggerPointCandidate origin_;
    SourceTranslatedBrushBoxTraceCandidate brush_;
    uint32_t handle_;

public:
    SourceMapDoorBrushCandidate(const SourceBspMapDoorTriggerCandidate& route, uint32_t handle,
        const SourceBspBrushOnlyTraceCandidate& collision, const SourcePreparedWorldRayCandidate& prepared)
        : origin_(route.origin), brush_(collision, prepared, route.model), handle_(handle) {}

    uint32_t handle() const { return handle_; }

    uint32_t clip(const SourceSolidTriggerSweepCandidate& hull, uint32_t mask) const {
        return brush_.trace(origin_, hull, mask).state.trace.contents;
    }
};

// Scan targets are admitted trigger brushes in the order returned by the map
// query. Finish is separate so the caller can service all pending relinks.
template<class Schedule>
std::vector<uint32_t> sourceScanSolidForMapDoors(SourceMapDoorRuntimeCandidate& map,
    uint32_t solid, const SourceSolidTriggerSweepCandidate& hull,
    const std::vector<const SourceMapDoorBrushCandidate*>& brushes, float now, Schedule schedule) {
    if (hull.isRay) throw std::invalid_argument("map door scan requires a solid box");
    std::vector<SourceSolidTriggerTargetCandidate> candidates;
    candidates.reserve(brushes.size());
    for (uint32_t i = 0; i < brushes.size(); ++i) candidates.push_back({i, 8, false, true, true, {}});
    // The collector visits each supplied trigger in order and dispatches only
    // after narrow-phase collection. All bound brushes use the brush route.
    auto contacts = sourceSolidTriggerContactsCandidate(hull, candidates,
        [](uint32_t) { return true; },
        [&](const auto& sweep, uint32_t index, uint32_t mask) { return brushes[index]->clip(sweep, mask); });
    map.beginScan(solid, schedule);
    for (auto& index : contacts) {
        index = brushes[index]->handle();
        map.reportContact(solid, index, now);
    }
    return contacts;
}
