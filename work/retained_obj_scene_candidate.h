#pragma once
#include "studio_obj_render_mesh_candidate.h"
#include "studio_obj_scale_candidate.h"
#include "scene_gpu_upload_candidate.h"
#include "scene_material_inputs_candidate.h"
// Imported mesh preview: material slots are retained, with explicit white
// unlit placeholders for slots without explicit opaque PNG assignments.
class RetainedObjSceneCandidate {
 StudioObjModelCandidate sourceModel_;StudioObjRenderMeshCandidate mesh_;std::vector<SceneGpuVertexCandidate> scaledVertices_;std::shared_ptr<const ModelRenderInputsCandidate> materials_;float scale_=1;
public:
 explicit RetainedObjSceneCandidate(const StudioObjModelCandidate& model,std::map<std::string,VtfRgbaImageCandidate> textures={},float scale=1):sourceModel_(model),mesh_(studioPrepareObjRenderMeshCandidate(model)),scaledVertices_(studioScaleObjVerticesCandidate(mesh_.vertices,scale)),scale_(scale){
  for(const auto& texture:textures){if(std::find(mesh_.materials.begin(),mesh_.materials.end(),texture.first)==mesh_.materials.end())throw std::runtime_error("OBJ texture material does not exist: "+texture.first);const auto& image=texture.second;if(!image.width||!image.height||uint64_t(image.width)*image.height*4!=image.rgba.size())throw std::runtime_error("OBJ texture dimensions invalid");for(size_t i=3;i<image.rgba.size();i+=4)if(image.rgba[i]!=255)throw std::runtime_error("OBJ transparent PNG rendering is not implemented");}auto inputs=std::make_shared<ModelRenderInputsCandidate>();for(const auto& name:mesh_.materials){ModelRenderMaterialCandidate material;material.source.syntax=parseVmtCandidate("\"UnlitGeneric\" { \"$nocull\" \"1\" }");auto assigned=textures.find(name);material.image=assigned==textures.end()?VtfRgbaImageCandidate{1,1,{255,255,255,255}}:std::move(assigned->second);inputs->materials.push_back(std::move(material));}materials_=std::move(inputs);
 }
 std::shared_ptr<const RetainedObjSceneCandidate> withTexture(size_t slot,VtfRgbaImageCandidate image)const{if(slot>=mesh_.materials.size())throw std::runtime_error("OBJ texture material slot invalid");if(!image.width||!image.height||uint64_t(image.width)*image.height*4!=image.rgba.size())throw std::runtime_error("OBJ texture dimensions invalid");for(size_t i=3;i<image.rgba.size();i+=4)if(image.rgba[i]!=255)throw std::runtime_error("OBJ transparent PNG rendering is not implemented");auto materials=std::make_shared<ModelRenderInputsCandidate>(*materials_);materials->materials.at(slot).image=std::move(image);auto next=std::make_shared<RetainedObjSceneCandidate>(*this);next->materials_=std::move(materials);return next;}
 const StudioObjModelCandidate& sourceModel()const{return sourceModel_;}
 const ModelRenderInputsCandidate& textureInputs()const{return *materials_;}
 float scale()const{return scale_;}
 std::shared_ptr<const RetainedObjSceneCandidate> withScale(float factor)const{auto vertices=studioScaleObjVerticesCandidate(mesh_.vertices,factor);auto next=std::make_shared<RetainedObjSceneCandidate>(*this);next->scaledVertices_=std::move(vertices);next->scale_=factor;return next;}
 const StudioObjRenderMeshCandidate& mesh()const{return mesh_;}
 template<class Prepared>void append(Prepared& prepared)const{
  auto& upload=prepared.upload;auto& inputs=prepared.materialInputs;
  if(upload.vertices.size()>UINT32_MAX||mesh_.vertices.size()>UINT32_MAX-upload.vertices.size()||upload.indices.size()>UINT32_MAX||mesh_.indices.size()>UINT32_MAX-upload.indices.size()||upload.materials.size()>UINT32_MAX||mesh_.materials.size()>UINT32_MAX-upload.materials.size())throw std::runtime_error("OBJ scene buffer overflow");
  const auto vertexCount=upload.vertices.size(),indexCount=upload.indices.size(),drawCount=upload.opaque.size(),slotCount=upload.materials.size(),entryCount=inputs.entries.size(),ownerCount=inputs.owners.size(),inputAlphaCount=inputs.alpha.size(),alphaCount=prepared.materialAlpha.size();
  try{
   upload.vertices.reserve(vertexCount+mesh_.vertices.size());upload.indices.reserve(indexCount+mesh_.indices.size());upload.materials.reserve(slotCount+mesh_.materials.size());upload.opaque.reserve(drawCount+mesh_.ranges.size());prepared.materialAlpha.reserve(alphaCount+mesh_.materials.size());auto group=inputs.appendOwned(materials_);
   for(size_t i=0;i<mesh_.materials.size();++i){upload.materials.push_back({group,static_cast<uint32_t>(i)});prepared.materialAlpha.push_back(1);}
   upload.vertices.insert(upload.vertices.end(),scaledVertices_.begin(),scaledVertices_.end());for(auto index:mesh_.indices)upload.indices.push_back(static_cast<uint32_t>(vertexCount)+index);
   for(size_t i=0;i<mesh_.ranges.size();++i){const auto& r=mesh_.ranges[i];upload.opaque.push_back({group,i,static_cast<uint32_t>(indexCount)+r.firstIndex,r.indexCount,r.material,static_cast<uint32_t>(slotCount)+r.material});}
  }catch(...){upload.vertices.resize(vertexCount);upload.indices.resize(indexCount);upload.opaque.resize(drawCount);upload.materials.resize(slotCount);prepared.materialAlpha.resize(alphaCount);inputs.entries.resize(entryCount);inputs.owners.resize(ownerCount);inputs.alpha.resize(inputAlphaCount);throw;}
 }
};



