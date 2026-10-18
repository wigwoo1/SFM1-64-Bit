#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>

struct VulkanMaterialDescriptorCountsCandidate {
    uint32_t maxSets = 0;
    uint32_t sampledImages = 0;
    uint32_t samplers = 0;
};

inline VulkanMaterialDescriptorCountsCandidate vulkanMaterialDescriptorCountsCandidate(
    size_t materialCount, bool withSelfIllumMask) {
    const size_t sampledImagesPerMaterial = withSelfIllumMask ? 3u : 2u;
    const size_t maximum = std::numeric_limits<uint32_t>::max();
    if (materialCount > maximum || materialCount > maximum / sampledImagesPerMaterial)
        throw std::overflow_error("material image descriptor count overflow");
    const auto count = static_cast<uint32_t>(materialCount);
    return {count, static_cast<uint32_t>(materialCount * sampledImagesPerMaterial), count};
}
