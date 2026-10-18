#pragma once
#include "session_render_scene_candidate.h"
#include "model_flex_deformation_candidate.h"
#include "software_model_flex_candidate.h"
#include "session_local_flex_controllers_candidate.h"
#include "mdl_direct_flex_evaluation_candidate.h"
#include "model_body_selection_candidate.h"
#include "sfm_model_geometry.h"
#include <map>
#include <filesystem>
#include <fstream>
#include <chrono>
struct StaticFlexStageProfileCandidate {double metadata=0,controllers=0,rules=0,deformation=0,copy=0,processing=0;};
// Owned metadata and immutable decoded morphs. Static temporal policy uses
// identical descriptor arrays for each sample; native history is unresolved.
class RetainedStaticFlexProviderCandidate {
 bool software_,tangents_;
 struct Metadata {uint32_t checksum,descriptors;size_t vertexCount;float scale;std::vector<MdlFlexControllerMetadata>controllers;std::vector<MdlFlexRuleCandidate>rules;std::vector<MdlMeshFlexCandidate>flexes;};
 std::map<uint32_t,VvdGeometryCandidate>outputs_;
 std::vector<std::filesystem::path>mounts_;std::map<std::string,Metadata>cache_;
 const Metadata&metadata(const std::string&name,const SfmModelGeometryCandidate&model){
  auto found=cache_.find(name);if(found!=cache_.end()){if(found->second.checksum!=model.checksum||found->second.vertexCount!=model.geometry.vertices.size())throw std::runtime_error("flex cached asset identity mismatch");return found->second;}
  std::string portable=name;for(char&c:portable)if(c=='\\')c='/';std::filesystem::path relative(portable);if(relative.empty()||relative.is_absolute()||relative.has_root_name()||portable.find(':')!=std::string::npos)throw std::runtime_error("invalid flex asset path");for(const auto&part:relative)if(part=="..")throw std::runtime_error("flex asset traversal");std::filesystem::path path;
  for(const auto&mount:mounts_){if(!std::filesystem::is_regular_file(mount/relative))continue;auto root=std::filesystem::weakly_canonical(mount),candidate=std::filesystem::weakly_canonical(mount/relative);auto inside=candidate.lexically_relative(root);if(inside.empty()||inside.is_absolute()||*inside.begin()=="..")throw std::runtime_error("flex asset outside mount");path=candidate;break;}if(path.empty())throw std::runtime_error("flex model missing: "+name);
  std::ifstream file(path,std::ios::binary);if(!file)throw std::runtime_error("flex model read failed");std::vector<uint8_t>bytes((std::istreambuf_iterator<char>(file)),{});auto controllers=readMdl49FlexMetadata(bytes);uint32_t checksum,descriptors;std::memcpy(&checksum,bytes.data()+8,4);std::memcpy(&descriptors,bytes.data()+260,4);if(checksum!=model.checksum)throw std::runtime_error("flex model checksum mismatch");
  Metadata value{checksum,descriptors,model.geometry.vertices.size(),readMdlFlexFixedScaleCandidate(bytes),std::move(controllers),readMdl49FlexRulesCandidate(bytes),readMdl49HalfMeshFlexesCandidate(bytes,model.geometry.vertices.size())};return cache_.emplace(name,std::move(value)).first->second;
 }
public:
 explicit RetainedStaticFlexProviderCandidate(std::vector<std::filesystem::path>mounts,bool software=false,bool tangents=false):software_(software),tangents_(tangents),mounts_(std::move(mounts)){}
 size_t metadataCount()const{return cache_.size();}
 void sampleInto(const Graph&graph,const SessionRenderModelCandidate&instance,const SfmModelGeometryCandidate&model,VvdGeometryCandidate&result,StaticFlexStageProfileCandidate*profile=nullptr){
  if(software_&&profile)throw std::runtime_error("software flex stage profiling unavailable");
  using Clock=std::chrono::steady_clock;auto last=profile?Clock::now():Clock::time_point{};auto mark=[&](double StaticFlexStageProfileCandidate::*field){if(profile){auto now=Clock::now();profile->*field=std::chrono::duration<double,std::milli>(now-last).count();last=now;}};
  const auto&data=metadata(instance.placement.asset,model);mark(&StaticFlexStageProfileCandidate::metadata);
  auto local=readSessionLocalFlexControllersCandidate(graph,instance.id,data.controllers);mark(&StaticFlexStageProfileCandidate::controllers);
  auto weights=evaluateDirectMdlFlexRulesCandidate(data.rules,data.descriptors,local);mark(&StaticFlexStageProfileCandidate::rules);
  auto body=modelBodySelectionCandidate(instance.body,model.bodyModelCounts,model.bodyBases);ModelFlexWorkProfileCandidate workProfile;if(software_)softwareModelFlexIntoCandidate(model.geometry,data.flexes,weights,weights,data.scale,[&](uint32_t b,uint32_t m){return body.at(b)==m;},tangents_,result);else deformModelFlexesIntoCandidate(model.geometry,data.flexes,weights,weights,data.scale,[&](uint32_t b,uint32_t m){return body.at(b)==m;},result,profile?&workProfile:nullptr);if(profile){profile->copy=workProfile.copy;profile->processing=workProfile.processing;}mark(&StaticFlexStageProfileCandidate::deformation);
 }
 VvdGeometryCandidate sample(const Graph&graph,const SessionRenderModelCandidate&instance,const SfmModelGeometryCandidate&model,StaticFlexStageProfileCandidate*profile=nullptr){VvdGeometryCandidate result;sampleInto(graph,instance,model,result,profile);return result;}
 const VvdGeometryCandidate&sampleBorrowed(const Graph&graph,const SessionRenderModelCandidate&instance,const SfmModelGeometryCandidate&model){auto&result=outputs_[instance.id];sampleInto(graph,instance,model,result);return result;}
};
