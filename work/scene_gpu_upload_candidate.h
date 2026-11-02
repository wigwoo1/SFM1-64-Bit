#pragma once
#include "scene_draw_plan_candidate.h"
#include "scene_gpu_vertex_candidate.h"
#include "scene_shading_attributes_candidate.h"
#include "scene_bsp_lightmap_candidate.h"
struct SceneGpuDrawCandidate {size_t model,range;uint32_t firstIndex,indexCount,materialIndex,materialSlot;};
struct SceneGpuMaterialAddressCandidate {size_t model;uint32_t material;};
struct SceneGpuUploadCandidate {
 std::vector<SceneGpuVertexCandidate>vertices;
 std::vector<uint32_t>indices;
 std::vector<SceneGpuMaterialAddressCandidate>materials;
 std::vector<SceneGpuDrawCandidate>opaque,translucent;
 // Empty only for explicit legacy/manual packets. Producers retain complete
 // aligned CPU records; availability is per vertex, never inferred from zeros.
 std::vector<SceneVertexShadingCandidate>vertexShading;
 // Shared CPU-only map metadata. Empty explicitly means no association; an
 // unavailable legacy-preview snapshot carries its reason, never white lighting.
 std::vector<SceneBspLightmapAssociationCandidate>bspLightmaps;
};
template<class Models,class Inputs,class Depth>
SceneGpuUploadCandidate prepareSceneGpuUploadCandidate(const Models&models,const Inputs&inputs,Depth depth){
 auto plan=sceneDrawPlanCandidate(models,inputs,depth);
 SceneGpuUploadCandidate upload;
 // Size the concatenated buffers once; repeated vector growth copied the
 // already assembled mesh several times on every animated frame.
 size_t vertexCount=0,indexCount=0,materialCount=0;
 for(size_t model=0;model<models.size();++model){if(!models[model].model.visible)continue;
  if(models[model].vertices.size()>UINT32_MAX-vertexCount||inputs[model].indices.size()>UINT32_MAX-indexCount||inputs[model].materials.size()>UINT32_MAX-materialCount)throw std::runtime_error("scene GPU buffer overflow");
  vertexCount+=models[model].vertices.size();indexCount+=inputs[model].indices.size();materialCount+=inputs[model].materials.size();
 }
 upload.vertices.reserve(vertexCount);upload.indices.reserve(indexCount);upload.materials.reserve(materialCount);
 upload.opaque.reserve(plan.opaque.size());upload.translucent.reserve(plan.translucent.size());
 upload.vertexShading.reserve(vertexCount);
 std::vector<uint32_t>indexStarts(models.size()),materialStarts(models.size());
 for(size_t model=0;model<models.size();++model){
  if(!models[model].model.visible)continue;
  if(models[model].vertices.size()>UINT32_MAX-upload.vertices.size()||inputs[model].indices.size()>UINT32_MAX-upload.indices.size())throw std::runtime_error("scene GPU buffer overflow");
  const auto vertexStart=uint32_t(upload.vertices.size());indexStarts[model]=uint32_t(upload.indices.size());
  if(inputs[model].materials.size()>UINT32_MAX-upload.materials.size())throw std::runtime_error("scene GPU material overflow");
  materialStarts[model]=uint32_t(upload.materials.size());
  for(size_t material=0;material<inputs[model].materials.size();++material)upload.materials.push_back({model,uint32_t(material)});
  uint32_t sourceVertex=0;
  for(const auto&v:models[model].vertices){auto shading=sceneVertexShadingCandidate(v,SceneShadingOriginCandidate::ModelGeometry,model,sourceVertex++);upload.vertices.push_back({v.position,v.normal,v.uv});upload.vertexShading.push_back(shading);}
  for(auto index:inputs[model].indices){if(index>=models[model].vertices.size())throw std::runtime_error("scene GPU vertex index");upload.indices.push_back(vertexStart+index);}
 }
 auto append=[&](const auto&source,auto&target){for(const auto&draw:source){const auto&r=inputs[draw.model].ranges[draw.range];target.push_back({draw.model,draw.range,indexStarts[draw.model]+r.firstIndex,r.indexCount,r.materialIndex,materialStarts[draw.model]+r.materialIndex});}};
 append(plan.opaque,upload.opaque);append(plan.translucent,upload.translucent);
 validateSceneShadingCandidate(upload);
 validateSceneBspLightmapsCandidate(upload);
 return upload;
}
