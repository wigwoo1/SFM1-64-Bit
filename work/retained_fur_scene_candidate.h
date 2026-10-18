#pragma once
#include "prepared_session_scene_candidate.h"
#include "fur_root_distribution_candidate.h"
#include "fur_scene_upload_candidate.h"
#include "model_body_selection_candidate.h"
#include <memory>
#include <iostream>
#include "fur_solid_material_candidate.h"
#include "fur_guide_worker_pool_candidate.h"
#include "fur_model_identity_candidate.h"
class RetainedFurSceneCandidate{
 uint32_t model_;std::optional<FurModelIdentityCandidate>identity_;size_t count_;float length_,width_,taper_;uint64_t seed_;std::unique_ptr<FurSurfaceAttachmentSetCandidate>attachment_;std::shared_ptr<const ModelRenderInputsCandidate>material_;std::unique_ptr<FurGuideWorkerPoolCandidate>workers_;
public:
 RetainedFurSceneCandidate(uint32_t model,size_t count,float length,float width,float taper,uint64_t seed,std::array<uint8_t,3>color={220,220,220},size_t workers=0,bool coverage=false):model_(model),count_(count),length_(length),width_(width),taper_(taper),seed_(seed),material_(coverage?createFurCoverageMaterialCandidate(color):createFurSolidMaterialCandidate(color)),workers_(workers?std::make_unique<FurGuideWorkerPoolCandidate>(workers):nullptr){}
 void prepare(const Graph&graph,const SessionRenderSceneCandidate&scene,const std::vector<SceneGeometryModelCandidate>&geometry,const std::function<const SfmModelGeometryCandidate&(const std::string&)>&asset,PreparedSessionSceneCandidate&prepared){
  if(!identity_)identity_.emplace(graph,model_);auto target=identity_->resolve(graph);if(!target)return;size_t index=0;for(;index<geometry.size();++index)if(geometry[index].model.id==*target)break;if(index==geometry.size()||!geometry[index].model.visible)return;const auto&instance=geometry[index];const auto&model=asset(instance.model.placement.asset);auto body=modelBodySelectionCandidate(instance.model.body,model.bodyModelCounts,model.bodyBases);FurTopologyCandidate topology{instance.model.placement.asset,model.geometry.checksum,model.geometry.lod,model.geometry.vertices.size(),{}};for(const auto&mesh:model.meshes)if(body.at(mesh.body)==mesh.model)topology.triangles.insert(topology.triangles.end(),mesh.triangles.begin(),mesh.triangles.end());
  std::unique_ptr<FurSurfaceAttachmentSetCandidate>nextAttachment;
  if(!attachment_){auto anchors=distributeFurRootsCandidate(furRestSurfaceCandidate(model.geometry),topology.triangles,count_,length_,seed_);nextAttachment=std::make_unique<FurSurfaceAttachmentSetCandidate>(topology,std::move(anchors));}
  const auto&attachment=attachment_?*attachment_:*nextAttachment;
  using Clock=std::chrono::steady_clock;auto guideStart=Clock::now();auto guides=workers_?attachment.sampleWith(topology,instance.vertices,*workers_):attachment.sample(topology,instance.vertices);auto guideEnd=Clock::now();GraphTransformHierarchy hierarchy(graph);auto camera=hierarchy.world(scene.camera);auto ribbons=prepareFurRibbonsCandidate(guides,{camera[0][3],camera[1][3],camera[2][3]},width_,taper_);auto ribbonEnd=Clock::now();
  auto&upload=prepared.upload;auto&inputs=prepared.materialInputs;
  const auto vertices=upload.vertices.size(),indices=upload.indices.size(),draws=upload.opaque.size(),slots=upload.materials.size(),entries=inputs.entries.size(),owners=inputs.owners.size(),inputAlpha=inputs.alpha.size(),alpha=prepared.materialAlpha.size();
  try{if(slots>=UINT32_MAX)throw std::runtime_error("fur material slot overflow");auto materialGroup=inputs.appendOwned(material_);upload.materials.push_back({materialGroup,0});prepared.materialAlpha.push_back(1.f);appendOpaqueFurRibbonsCandidate(upload,ribbons,static_cast<uint32_t>(slots));}
  catch(...){upload.vertices.resize(vertices);upload.indices.resize(indices);upload.opaque.resize(draws);upload.materials.resize(slots);prepared.materialAlpha.resize(alpha);inputs.entries.resize(entries);inputs.owners.resize(owners);inputs.alpha.resize(inputAlpha);throw;}
  auto appendEnd=Clock::now();auto milliseconds=[](auto a,auto b){return std::chrono::duration<double,std::milli>(b-a).count();};prepared.profile.furGuides=milliseconds(guideStart,guideEnd);prepared.profile.furRibbons=milliseconds(guideEnd,ribbonEnd);prepared.profile.furAppend=milliseconds(ribbonEnd,appendEnd);
  if(nextAttachment)attachment_=std::move(nextAttachment);
  std::cout<<"fur_scene_strands="<<guides.size()<<" vertices="<<ribbons.vertices.size()<<" material=solid_fur prototype=1\n";
 }
};
