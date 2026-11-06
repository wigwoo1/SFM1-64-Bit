#pragma once
#include "model_render_inputs_candidate.h"
#include "model_body_selection_candidate.h"
#include <functional>
#include <tuple>
#include <filesystem>
// Fixed mount order/mip for the cache lifetime. Failed loads are never retained.
class SceneRenderInputsCacheCandidate {
 using Key=std::tuple<std::string,uint32_t,uint32_t,std::string,uint32_t,std::filesystem::path>;
 std::map<Key,ModelRenderInputsCandidate>entries_;
 std::vector<std::filesystem::path>mounts_;
 uint32_t mip_;
 bool hdr2Materials_;
 std::function<const SfmModelGeometryCandidate&(const std::string&)>asset_;
 // Optional immutable asset-container identity (for map-local inline names).
 // The caller keeps that source context stable throughout each get/sample.
 std::function<std::filesystem::path(const std::string&)>scope_;
 size_t hits_=0;
public:
 SceneRenderInputsCacheCandidate(std::vector<std::filesystem::path>mounts,uint32_t mip,std::function<const SfmModelGeometryCandidate&(const std::string&)>asset,std::function<std::filesystem::path(const std::string&)>scope={},bool hdr2Materials=false):mounts_(std::move(mounts)),mip_(mip),hdr2Materials_(hdr2Materials),asset_(std::move(asset)),scope_(std::move(scope)){if(!asset_||mounts_.empty())throw std::runtime_error("scene render inputs loader/mounts required");}
 const ModelRenderInputsCandidate&get(const std::string&name,uint32_t skin,uint32_t body,const std::string&overrideKey={},const std::function<void(const std::filesystem::path&,VmtNodeCandidate&)>&applyOverrides={},uint32_t textureFrame=0){
  if(bool(applyOverrides)!=!overrideKey.empty())throw std::runtime_error("scene material override key/callback mismatch");
  Key key{name,skin,body,overrideKey,textureFrame,scope_?scope_(name):std::filesystem::path{}};auto found=entries_.find(key);if(found!=entries_.end()){++hits_;return found->second;}
  const auto&source=asset_(name);
  auto input=prepareModelRenderInputsCandidate(source,skin,mounts_,mip_,modelBodySelectionCandidate(body,source.bodyModelCounts,source.bodyBases),applyOverrides,textureFrame);
  if(hdr2Materials_){
   for(auto& material:input.materials){
    auto& source=material.source;
    source.lightwarp=resolveLightwarpTexturesCandidate(source.syntax,mounts_,source.missingTextureReferences);
    if(source.lightwarp.warp.empty())continue;
    auto prepareAuxiliary=[&](const std::filesystem::path& path,const char* parameter,uint32_t mip){
     if(path.empty())return ModelPreparedTextureCandidate{};
     try{
      auto bytes=readBinaryAssetCandidate(path);
      const auto layout=readVtfTextureLayoutCandidate(bytes);
      const auto frame=layout.frameCount==1?0u:textureFrame;
      auto texture=prepareModelTextureSnapshotCandidate(vmtValuesCandidate(source.syntax,parameter).at(0),bytes,mip,frame);
      if(texture.cpu)return texture;
      source.missingTextureReferences.push_back(path.filename().string()+" (unsupported HDR2 texture)");
     }catch(const std::runtime_error& error){
      source.missingTextureReferences.push_back(path.filename().string()+" ("+error.what()+")");
     }
     return ModelPreparedTextureCandidate{};
    };
    material.lightwarpInputs.warp=prepareAuxiliary(source.lightwarp.warp,"$lightwarptexture",0);
    if(material.lightwarpInputs.warp.cpu){
     material.lightwarpInputs.scalarSkyPhong=scalarSkyPhongCandidate(source.syntax);
     material.lightwarpInputs.phongExponentControl=phongExponentControlCandidate(source.syntax);
     material.lightwarpInputs.normal=prepareAuxiliary(source.lightwarp.normal,"$bumpmap",0);
     material.lightwarpInputs.exponent=prepareAuxiliary(source.lightwarp.exponent,"$phongexponenttexture",0);
    }
   }
  }
  return entries_.emplace(std::move(key),std::move(input)).first->second;
 }
 size_t size()const{return entries_.size();}size_t hits()const{return hits_;}
};
