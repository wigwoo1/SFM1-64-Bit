#pragma once
#include "model_render_inputs_candidate.h"
#include "model_body_selection_candidate.h"
#include <functional>
#include <tuple>
// Fixed mount order/mip for the cache lifetime. Failed loads are never retained.
class SceneRenderInputsCacheCandidate {
 using Key=std::tuple<std::string,uint32_t,uint32_t,std::string,uint32_t>;
 std::map<Key,ModelRenderInputsCandidate>entries_;
 std::vector<std::filesystem::path>mounts_;
 uint32_t mip_;
 std::function<const SfmModelGeometryCandidate&(const std::string&)>asset_;
 size_t hits_=0;
public:
 SceneRenderInputsCacheCandidate(std::vector<std::filesystem::path>mounts,uint32_t mip,std::function<const SfmModelGeometryCandidate&(const std::string&)>asset):mounts_(std::move(mounts)),mip_(mip),asset_(std::move(asset)){if(!asset_||mounts_.empty())throw std::runtime_error("scene render inputs loader/mounts required");}
 const ModelRenderInputsCandidate&get(const std::string&name,uint32_t skin,uint32_t body,const std::string&overrideKey={},const std::function<void(const std::filesystem::path&,VmtNodeCandidate&)>&applyOverrides={},uint32_t textureFrame=0){
  if(bool(applyOverrides)!=!overrideKey.empty())throw std::runtime_error("scene material override key/callback mismatch");
  Key key{name,skin,body,overrideKey,textureFrame};auto found=entries_.find(key);if(found!=entries_.end()){++hits_;return found->second;}
  const auto&source=asset_(name);
  auto input=prepareModelRenderInputsCandidate(source,skin,mounts_,mip_,modelBodySelectionCandidate(body,source.bodyModelCounts,source.bodyBases),applyOverrides,textureFrame);
  return entries_.emplace(std::move(key),std::move(input)).first->second;
 }
 size_t size()const{return entries_.size();}size_t hits()const{return hits_;}
};
