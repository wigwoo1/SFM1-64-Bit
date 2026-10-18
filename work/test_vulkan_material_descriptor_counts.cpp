#include "vulkan_material_descriptor_counts_candidate.h"
#include <cassert>
#include <iostream>

int main() {
    const auto empty = vulkanMaterialDescriptorCountsCandidate(0, false);
    assert(empty.maxSets == 0 && empty.sampledImages == 0 && empty.samplers == 0);

    const auto ordinary = vulkanMaterialDescriptorCountsCandidate(59, false);
    assert(ordinary.maxSets == 59 && ordinary.sampledImages == 118 && ordinary.samplers == 59);
    const auto masked = vulkanMaterialDescriptorCountsCandidate(59, true);
    assert(masked.maxSets == 59 && masked.sampledImages == 177 && masked.samplers == 59);

    constexpr size_t max = std::numeric_limits<uint32_t>::max();
    const auto ordinaryBoundary = vulkanMaterialDescriptorCountsCandidate(max / 2, false);
    assert(ordinaryBoundary.sampledImages == uint32_t((max / 2) * 2));
    const auto maskedBoundary = vulkanMaterialDescriptorCountsCandidate(max / 3, true);
    assert(maskedBoundary.sampledImages == uint32_t((max / 3) * 3));

    auto rejects = [](size_t count, bool mask) {
        try { (void)vulkanMaterialDescriptorCountsCandidate(count, mask); }
        catch (const std::overflow_error&) { return true; }
        return false;
    };
    assert(rejects(max / 2 + 1, false));
    assert(rejects(max / 3 + 1, true));
    assert(rejects(max + size_t(1), false));

    std::cout << "Vulkan material descriptor counts passed empty=1 two-texture=1 optional-mask=1 overflow-boundaries=1\n";
}
