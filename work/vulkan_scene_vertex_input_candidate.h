#pragma once
#include "scene_gpu_vertex_candidate.h"
#include "dependencies/vulkan-headers-6802bb4733b6/Vulkan-Headers-6802bb4733b63ed5efd3adb308a6c885ef180ea1/include/vulkan/vulkan_core.h"
#include <cstddef>

inline VkVertexInputBindingDescription vulkanSceneVertexBindingCandidate() {
    return {0, sizeof(SceneGpuVertexCandidate), VK_VERTEX_INPUT_RATE_VERTEX};
}
inline std::array<VkVertexInputAttributeDescription, 4> vulkanSceneVertexAttributesCandidate() {
    static_assert(offsetof(SceneGpuVertexCandidate, position) == 0);
    static_assert(offsetof(SceneGpuVertexCandidate, normal) == 12);
    static_assert(offsetof(SceneGpuVertexCandidate, uv) == 24);
    static_assert(offsetof(SceneGpuVertexCandidate, displacementAlpha) == 32);
    return {{{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
             {2, 0, VK_FORMAT_R32G32_SFLOAT, 24},
             {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12},
             {3, 0, VK_FORMAT_R32_SFLOAT, 32}}};
}
