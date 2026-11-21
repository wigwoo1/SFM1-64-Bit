#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>

enum class SpatialFixtureOperation : uint32_t { none, move, replaceLists, destroy, create };

struct SpatialFixtureMutation {
    SpatialFixtureOperation operation = SpatialFixtureOperation::none;
    uint32_t handle = 0;
    uint32_t removeMask = 0;
    uint32_t insertMask = 0;
    uint64_t entityId = 0;
    std::array<float, 6> bounds{};

    void validate() const {
        if (operation > SpatialFixtureOperation::create || handle >= 4096 ||
            removeMask > 65535 || insertMask > 65535) {
            throw std::runtime_error("invalid spatial fixture command");
        }
        if (operation == SpatialFixtureOperation::move || operation == SpatialFixtureOperation::create) {
            for (float value : bounds) {
                if (!std::isfinite(value)) throw std::runtime_error("nonfinite spatial fixture bounds");
            }
            for (size_t axis = 0; axis < 3; ++axis) {
                if (bounds[axis] > bounds[axis + 3]) throw std::runtime_error("inverted spatial fixture bounds");
            }
        }
    }
};

static_assert(sizeof(SpatialFixtureMutation) == 48, "mutation input layout");
