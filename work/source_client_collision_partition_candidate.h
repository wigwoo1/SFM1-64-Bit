#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>

struct SourceClientCollisionPoseCandidate {
    int16_t modelIndex = 0;
    std::array<float, 3> origin{};
    std::array<float, 3> angles{};
};

struct SourceClientCollisionPartitionCandidate {
    static constexpr uint16_t invalidHandle = 0xffff;
    static constexpr uint32_t dirtyBounds = 0x4000;
    static constexpr uint32_t dirtyPartition = 0x8000;
    static constexpr uint32_t partitionWhenNonsolid = 0x40000;
    static constexpr uint16_t notSolid = 0x04;
    static constexpr uint16_t trigger = 0x08;
    static constexpr uint16_t forceWorldAligned = 0x40;
    static constexpr uint16_t useTriggerBounds = 0x80;
    static constexpr uint8_t solidNone = 0;
    static constexpr uint8_t solidBBox = 2;
    static constexpr uint8_t solidCustom = 5;
    static constexpr uint8_t solidVPhysics = 6;
    static constexpr uint8_t useObbCollisionBounds = 0;
    static constexpr uint8_t useSpecifiedBounds = 3;
    static constexpr uint8_t useGameCodeBounds = 4;

    uint32_t entityFlags = 0;
    uint16_t handle = invalidHandle;
    uint16_t solidFlags = 0;
    uint8_t solidType = 0;
    uint8_t triggerBloat = 0;
    uint8_t surroundingBoundsType = 0;
    float radius = 0;
    std::array<float, 6> collisionBounds{};
    std::array<float, 6> specifiedBounds{};
    std::array<float, 6> cachedRelativeBounds{};

    bool solid() const { return solidType != solidNone && !(solidFlags & notSolid); }

    std::array<float, 3> collisionAngles(const SourceClientCollisionPoseCandidate& pose) const {
        if ((solidFlags & forceWorldAligned) || solidType == solidNone || solidType == solidBBox) return {};
        return pose.angles;
    }
};

struct SourceClientPartitionListsCandidate {
    uint16_t handle;
    uint32_t removeMask;
    uint32_t insertMask;
};

// Cached model-info type, with noModel denoting a null model pointer.
enum class SourceClientCollisionModelKindCandidate : int32_t {
    noModel = -1, other = 0, brush = 1, studio = 3,
};

inline SourceClientPartitionListsCandidate sourceClientBasePartitionListsCandidate(
    const SourceClientCollisionPartitionCandidate& state, int16_t modelIndex,
    SourceClientCollisionModelKindCandidate modelKind, bool extraPartitionList) {
    const bool collisionModel = modelKind == SourceClientCollisionModelKindCandidate::brush ||
                                modelKind == SourceClientCollisionModelKindCandidate::studio;
    const bool clientSolid = modelIndex != 0 && state.solid() && collisionModel &&
                             (modelIndex == 1 || state.radius != 0.f);
    constexpr uint32_t clientLists = 0x28c;
    constexpr uint32_t clientNonStatic = 0x80;
    constexpr uint32_t clientSolidList = 0x04;
    constexpr uint32_t extraList = 0x200;
    return {state.handle, clientLists,
        clientNonStatic | (clientSolid ? clientSolidList : 0u) | (extraPartitionList ? extraList : 0u)};
}

struct SourceClientPartitionMoveCandidate {
    uint16_t handle;
    // Already includes the client padding. The partition applies its own padding.
    std::array<float, 6> bounds;
};

// The pose must come from the resolved client transform. Dirty surrounding
// bounds are supported for translated OBBs and specified relative bounds.
inline std::optional<SourceClientPartitionMoveCandidate> sourceClientPartitionMoveCandidate(
    SourceClientCollisionPartitionCandidate& state, const SourceClientCollisionPoseCandidate& pose) {
    using Partition = SourceClientCollisionPartitionCandidate;
    if (!(state.entityFlags & Partition::dirtyPartition)) return std::nullopt;
    if (state.handle == Partition::invalidHandle ||
        (!state.solid() && !(state.solidFlags & Partition::trigger) &&
         !(state.entityFlags & Partition::partitionWhenNonsolid))) {
        state.entityFlags &= ~Partition::dirtyPartition;
        return std::nullopt;
    }

    auto relative = state.cachedRelativeBounds;
    bool refreshed = false;
    std::array<float, 6> bounds{};
    const bool pointSized = state.radius == 0.f;
    if (pointSized) {
        for (std::size_t axis = 0; axis < bounds.size(); ++axis) bounds[axis] = pose.origin[axis % 3];
    } else {
        const bool dirty = (state.entityFlags & Partition::dirtyBounds) != 0;
        if (!dirty) {
            for (std::size_t axis = 0; axis < bounds.size(); ++axis) {
                bounds[axis] = pose.origin[axis % 3] + relative[axis];
            }
        } else {
            if (state.solidType == Partition::solidCustom && state.surroundingBoundsType != Partition::useGameCodeBounds) {
                for (std::size_t axis = 0; axis < bounds.size(); ++axis) bounds[axis] = pose.origin[axis % 3];
            } else if (state.surroundingBoundsType == Partition::useSpecifiedBounds) {
                for (std::size_t axis = 0; axis < bounds.size(); ++axis) {
                    bounds[axis] = pose.origin[axis % 3] + state.specifiedBounds[axis];
                }
            } else if (state.surroundingBoundsType == Partition::useObbCollisionBounds &&
                       state.solidType != Partition::solidVPhysics &&
                       state.collisionAngles(pose) == std::array<float, 3>{}) {
                for (std::size_t axis = 0; axis < bounds.size(); ++axis) {
                    bounds[axis] = pose.origin[axis % 3] + state.collisionBounds[axis];
                }
                if (state.solidFlags & Partition::useTriggerBounds) {
                    const float bloat = state.triggerBloat;
                    bounds[0] -= bloat;
                    bounds[1] -= bloat;
                    bounds[3] += bloat;
                    bounds[4] += bloat;
                    bounds[5] += bloat * 0.5f;
                }
            } else {
                throw std::runtime_error("unsupported dirty client collision bounds mode");
            }
            for (std::size_t axis = 0; axis < bounds.size(); ++axis) {
                relative[axis] = bounds[axis] - pose.origin[axis % 3];
            }
            refreshed = true;
        }
    }

    for (std::size_t axis = 0; axis < 3; ++axis) {
        if (!std::isfinite(bounds[axis]) || !std::isfinite(bounds[axis + 3]) ||
            bounds[axis] > bounds[axis + 3]) {
            throw std::runtime_error("invalid client collision partition bounds");
        }
        if (!pointSized) {
            bounds[axis] -= 1.f;
            bounds[axis + 3] += 1.f;
        }
    }
    if (refreshed) {
        for (float value : relative) {
            if (!std::isfinite(value)) throw std::runtime_error("invalid client collision bounds cache");
        }
        state.cachedRelativeBounds = relative;
        state.entityFlags &= ~Partition::dirtyBounds;
    }
    state.entityFlags &= ~Partition::dirtyPartition;
    return SourceClientPartitionMoveCandidate{state.handle, bounds};
}
