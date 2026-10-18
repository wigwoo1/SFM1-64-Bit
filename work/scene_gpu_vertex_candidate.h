#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>
struct SceneGpuVertexCandidate {std::array<float,3>position,normal;std::array<float,2>uv;float displacementAlpha=0.f;};
static_assert(sizeof(SceneGpuVertexCandidate)==36,"Vulkan vertex stride");
inline bool sceneGpuVertexFiniteCandidate(const SceneGpuVertexCandidate&vertex){
 static_assert(sizeof(float)==4&&std::numeric_limits<float>::is_iec559,"IEEE binary32 required");
 static_assert(std::is_trivially_copyable<SceneGpuVertexCandidate>::value,"wire copy required");
 std::array<uint32_t,9>bits{};std::memcpy(bits.data(),&vertex,sizeof(vertex));
 bool finite=true;for(auto value:bits)finite&=(value&0x7f800000u)!=0x7f800000u;return finite;
}
