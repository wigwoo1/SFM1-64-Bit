#pragma once
#include "fur_ribbon_geometry_candidate.h"
#include "scene_gpu_upload_candidate.h"
// Opaque prototype bridge using an explicitly selected existing material slot.
// Fur materials, alpha coverage and transparent strand sorting remain separate.
inline bool appendOpaqueFurRibbonsCandidate(SceneGpuUploadCandidate&scene,const FurRibbonGeometryCandidate&fur,uint32_t materialSlot,size_t range=0){
 validateSceneShadingCandidate(scene);const auto shadingCount=scene.vertexShading.size();
 if(materialSlot>=scene.materials.size())throw std::runtime_error("fur scene material slot");if(fur.indices.empty())return false;if(fur.indices.size()%3)throw std::runtime_error("fur triangle index count");if(scene.vertices.size()>UINT32_MAX||fur.vertices.size()>UINT32_MAX-scene.vertices.size()||scene.indices.size()>UINT32_MAX||fur.indices.size()>UINT32_MAX-scene.indices.size())throw std::runtime_error("fur scene buffer overflow");
 for(auto index:fur.indices)if(index>=fur.vertices.size())throw std::runtime_error("fur ribbon index outside vertices");for(const auto&vertex:fur.vertices)if(!sceneGpuVertexFiniteCandidate(vertex))throw std::runtime_error("nonfinite fur upload vertex");
 const auto address=scene.materials[materialSlot];uint32_t base=static_cast<uint32_t>(scene.vertices.size()),first=static_cast<uint32_t>(scene.indices.size());scene.vertices.reserve(scene.vertices.size()+fur.vertices.size());scene.indices.reserve(scene.indices.size()+fur.indices.size());scene.opaque.reserve(scene.opaque.size()+1);
 const auto draws=scene.opaque.size();scene.vertexShading.reserve(size_t(base)+fur.vertices.size());
 try{if(scene.vertexShading.empty())scene.vertexShading.resize(base);for(size_t i=0;i<fur.vertices.size();++i)scene.vertexShading.push_back(sceneVertexShadingCandidate(fur.vertices[i],SceneShadingOriginCandidate::GeneratedGeometry,address.model,static_cast<uint32_t>(i)));
 scene.vertices.insert(scene.vertices.end(),fur.vertices.begin(),fur.vertices.end());scene.indices.insert(scene.indices.end(),fur.indices.begin(),fur.indices.end());for(size_t i=first;i<scene.indices.size();++i)scene.indices[i]+=base;scene.opaque.push_back({address.model,range,first,static_cast<uint32_t>(fur.indices.size()),address.material,materialSlot});validateSceneShadingCandidate(scene);return true;
 }catch(...){scene.vertexShading.resize(shadingCount);scene.vertices.resize(base);scene.indices.resize(first);scene.opaque.resize(draws);throw;}
}
