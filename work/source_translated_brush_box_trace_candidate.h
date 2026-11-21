#pragma once

#include "source_solid_trigger_scan_candidate.h"
#include "source_swept_world_trace_candidate.h"

#include <algorithm>

// A zero-angle inline model consisting of one axial box brush. Validate its
// BSP cover before using the single-brush sweep in place of a general walk.
class SourceTranslatedBrushBoxTraceCandidate {
    const SourceBspBrushOnlyTraceCandidate& world_;
    int32_t root_;
    SourceTriggerBoundsCandidate bounds_{};
    uint32_t contents_ = 0;

public:
    SourceTranslatedBrushBoxTraceCandidate(const SourceBspBrushOnlyTraceCandidate& world,
        const SourcePreparedWorldRayCandidate& prepared, std::size_t model)
        : world_(world), root_(prepared.modelRoot(model)) {
        std::array<bool, 6> faces{};
        int32_t next = root_;
        const auto isEmptyLeaf = [&](int32_t child) {
            if (child >= 0) return false;
            const auto leaf = static_cast<std::size_t>(-1 - int64_t(child));
            return world.data.leafContents.at(leaf) == 0 && world.data.leafBrushes.at(leaf).empty() &&
                prepared.leafRefs().at(leaf).empty();
        };
        for (std::size_t i = 0; i < faces.size(); ++i) {
            if (next < 0) throw std::runtime_error("inline box partition ended before six faces");
            const auto& node = world.nodes.at(static_cast<std::size_t>(next));
            const auto& plane = world.planes.at(node.plane);
            const auto axis = plane.type;
            if (axis >= 3) throw std::runtime_error("inline box partition is not axial");
            SourceTriggerPointCandidate normal{};
            normal[axis] = 1.f;
            if (plane.normal != normal) throw std::runtime_error("unsupported inline box plane orientation");
            const bool emptyFront = isEmptyLeaf(node.children[0]);
            const bool emptyBack = isEmptyLeaf(node.children[1]);
            if (emptyFront == emptyBack) throw std::runtime_error("inline box lacks a unique inside child");
            const auto face = axis + (emptyFront ? 3u : 0u);
            if (faces[face]) throw std::runtime_error("duplicate inline box partition face");
            faces[face] = true;
            if (emptyFront) bounds_.max[axis] = plane.distance;
            else bounds_.min[axis] = plane.distance;
            next = node.children[emptyFront ? 1 : 0];
        }
        if (next >= 0) throw std::runtime_error("inline box partition has more than six faces");
        const auto leaf = static_cast<std::size_t>(-1 - int64_t(next));
        const auto& brushes = world.data.leafBrushes.at(leaf);
        if (brushes.size() != 1 || !prepared.leafRefs().at(leaf).empty()) {
            throw std::runtime_error("inline collision model is not one box");
        }
        const auto index = brushes[0];
        const auto& record = world.records.at(index);
        contents_ = world.data.brushes.at(index).contents;
        if (!record.optimized || !(contents_ & 1u) || world.data.leafContents.at(leaf) != contents_) {
            throw std::runtime_error("inline box contents or collision record differ from its leaf");
        }
        SourceTriggerBoundsCandidate brushBounds;
        std::memcpy(brushBounds.min.data(), record.box.data(), 12);
        std::memcpy(brushBounds.max.data(), record.box.data()+16, 12);
        if (brushBounds.min != bounds_.min || brushBounds.max != bounds_.max) {
            throw std::runtime_error("inline box brush faces differ from its partition");
        }
    }

    SourceSweptWorldTraceCandidate trace(const SourceTriggerPointCandidate& brushOrigin,
        const SourceSolidTriggerSweepCandidate& hull, uint32_t mask) const {
        if (hull.isRay || (mask & 0x2000u)) throw std::runtime_error("unsupported inline box hull query");
        SourceTriggerPointCandidate center{}, end{}, originalStart{}, leafExtents{};
        for (std::size_t axis = 0; axis < 3; ++axis) {
            center[axis] = hull.center[axis] - brushOrigin[axis];
            end[axis] = center[axis] + hull.delta[axis];
            originalStart[axis] = hull.center[axis] + hull.offset[axis];
            leafExtents[axis] = hull.extents[axis] + 1.f;
            if (!std::isfinite(center[axis]) || !std::isfinite(end[axis]) || !std::isfinite(originalStart[axis]) ||
                !std::isfinite(leafExtents[axis]) || hull.extents[axis] < 0.f) {
                throw std::runtime_error("invalid inline box hull query");
            }
        }
        SourceSweptWorldTraceCandidate result;
        auto& hit = result.state.trace;
        if (hull.swept) {
            if (mask & contents_) {
                sourceOptimizedBoxLineRayCandidate(bounds_.min, bounds_.max, center, end,
                    contents_, hit, nullptr, &hull.delta, &hull.extents);
            }
        } else {
            bool overlaps = true;
            for (std::size_t axis = 0; axis < 3; ++axis) {
                if (center[axis] + hull.extents[axis] < bounds_.min[axis] ||
                    center[axis] - hull.extents[axis] > bounds_.max[axis]) overlaps = false;
            }
            if (overlaps && (mask & contents_)) {
                hit.contents = contents_;
            } else {
                const auto leaves = sourceStationaryLeafSelectionAtRootCandidate(world_, root_, center, leafExtents);
                overlaps = std::all_of(leaves.leaves.begin(), leaves.leaves.end(),
                    [&](uint32_t leaf) { return (world_.data.leafContents.at(leaf) & 1u) != 0; });
            }
            if (overlaps) {
                hit.fraction = 0.f;
                hit.leftSolid = 1.f;
                hit.startSolid = true;
                hit.allSolid = true;
            }
        }
        sourceWorldTracePositionsCandidate(originalStart, hull.delta, result);
        // The translated entry uses the pre-translation world center plus
        // the original start offset, including the sign of a resulting zero.
        if (hit.leftSolid == 0.f) result.startPosition = originalStart;
        if (hit.leftSolid == 1.f) result.endPosition = originalStart;
        return result;
    }
};
