#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

// CPU-only, indexed exactly like the legacy vertex buffer. These records are
// never copied into the 36-byte Vulkan vertex ABI. Zero values with no validity
// flag are placeholders, not authored normals, tangents or lighting.
enum class SceneShadingOriginCandidate:uint32_t {Unavailable,ModelGeometry,WorldGeometry,ImportedGeometry,GeneratedGeometry};
struct SceneVertexShadingCandidate {
    std::array<float,3>normal{};
    std::array<float,4>tangent{};
    std::array<float,3>previewLightColor{};
    size_t model=std::numeric_limits<size_t>::max();
    uint32_t sourceVertex=UINT32_MAX;
    SceneShadingOriginCandidate origin=SceneShadingOriginCandidate::Unavailable;
    bool hasNormal=false,hasTangent=false,hasPreviewLightColor=false;
};
inline void validateSceneVertexShadingCandidate(const SceneVertexShadingCandidate&v){
    const auto finite=[](const auto&values){for(float value:values)if(!std::isfinite(value))throw std::runtime_error("nonfinite CPU shading attribute");};
    if(v.origin==SceneShadingOriginCandidate::Unavailable){
        if(v.hasNormal||v.hasTangent||v.hasPreviewLightColor||v.model!=std::numeric_limits<size_t>::max()||v.sourceVertex!=UINT32_MAX)throw std::runtime_error("unavailable CPU shading identity/flags");
        return;
    }
    if(uint32_t(v.origin)>uint32_t(SceneShadingOriginCandidate::GeneratedGeometry)||!v.hasNormal||v.model==std::numeric_limits<size_t>::max()||v.sourceVertex==UINT32_MAX)throw std::runtime_error("CPU shading source identity");
    finite(v.normal);if(v.hasTangent)finite(v.tangent);if(v.hasPreviewLightColor)finite(v.previewLightColor);
}
template<class Vertex,class=void>struct SceneShadingHasTangentCandidate:std::false_type{};
template<class Vertex>struct SceneShadingHasTangentCandidate<Vertex,std::void_t<decltype(std::declval<const Vertex&>().tangent),decltype(std::declval<const Vertex&>().hasTangent)>>:std::true_type{};
template<class Vertex>SceneVertexShadingCandidate sceneVertexShadingCandidate(const Vertex&v,SceneShadingOriginCandidate origin,size_t model,uint32_t sourceVertex){
    SceneVertexShadingCandidate out;out.normal=v.normal;out.hasNormal=true;out.model=model;out.sourceVertex=sourceVertex;out.origin=origin;
    if constexpr(SceneShadingHasTangentCandidate<Vertex>::value){out.hasTangent=v.hasTangent;if(out.hasTangent)out.tangent=v.tangent;}
    validateSceneVertexShadingCandidate(out);return out;
}
template<class Upload>void validateSceneShadingCandidate(const Upload&upload,bool requirePresent=false){
    if(upload.vertexShading.empty()){if(requirePresent&&!upload.vertices.empty())throw std::runtime_error("CPU shading attributes absent");return;}
    if(upload.vertexShading.size()!=upload.vertices.size())throw std::runtime_error("CPU shading vertex coverage");
    for(const auto&v:upload.vertexShading)validateSceneVertexShadingCandidate(v);
    for(const auto*list:{&upload.opaque,&upload.translucent})for(const auto&draw:*list){
        if(draw.firstIndex>upload.indices.size()||draw.indexCount>upload.indices.size()-draw.firstIndex)throw std::runtime_error("CPU shading draw span");
        for(size_t n=0;n<draw.indexCount;++n){auto index=upload.indices.at(size_t(draw.firstIndex)+n);if(index>=upload.vertexShading.size())throw std::runtime_error("CPU shading vertex index");const auto&v=upload.vertexShading[index];if(v.origin!=SceneShadingOriginCandidate::Unavailable&&v.model!=draw.model)throw std::runtime_error("CPU shading draw source mismatch");}
    }
}
// A future lit consumer must explicitly require the channels its shader needs;
// legacy/unlit packets with absent or unavailable data cannot pass this gate.
template<class Upload,class Draw>void requireSceneDrawShadingCandidate(const Upload&upload,const Draw&draw,bool tangentRequired){
    validateSceneShadingCandidate(upload,true);
    if(draw.firstIndex>upload.indices.size()||draw.indexCount>upload.indices.size()-draw.firstIndex)throw std::runtime_error("required shading draw span");
    for(size_t n=0;n<draw.indexCount;++n){const auto&v=upload.vertexShading.at(upload.indices.at(size_t(draw.firstIndex)+n));if(!v.hasNormal||(tangentRequired&&!v.hasTangent)||v.model!=draw.model)throw std::runtime_error("required CPU shading channel unavailable");}
}
