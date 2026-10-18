#pragma once
#include "session_render_light_candidate.h"
#include "mounted_model_material_candidate.h"
#include "read_binary_asset_candidate.h"
#include "vtf_block_decode.h"
#include "projected_light_texture_frame_candidate.h"
#include <memory>
#include <map>
struct ProjectedLightTextureInputCandidate {
 std::filesystem::path path;
 uint32_t frame=0,mip=0;
 std::shared_ptr<const VtfRgbaImageCandidate>image;
};
class ProjectedLightTextureInputsCandidate {
 struct Asset {std::vector<uint8_t>bytes;uint32_t frames,mips;};
 std::map<std::filesystem::path,Asset>assets;
 std::map<std::tuple<std::filesystem::path,uint32_t,uint32_t>,std::shared_ptr<const VtfRgbaImageCandidate>>images;
public:
 ProjectedLightTextureInputCandidate prepare(const SessionRenderLightCandidate&light,const std::vector<std::filesystem::path>&mounts,uint32_t mip=0){
  if(light.fields.textureName.empty())return {};
  auto path=findMountedMaterialCandidate(mounts,materialRelativePathCandidate(light.fields.textureName,".vtf"));
  if(path.empty())throw std::runtime_error("projected light texture missing: "+light.fields.textureName);
  path=std::filesystem::weakly_canonical(path);auto found=assets.find(path);
  if(found==assets.end()){auto bytes=readBinaryAssetCandidate(path);auto layout=readVtfTextureLayoutCandidate(bytes);if(!layout.frameCount||layout.frameCount>INT32_MAX||layout.mips.empty())throw std::runtime_error("projected light texture layout");found=assets.emplace(path,Asset{std::move(bytes),layout.frameCount,uint32_t(layout.mips.size())}).first;}
  auto&asset=found->second;
  auto frame=projectedLightTextureFrameCandidate(light.fields.animationTimeWord,light.fields.floatWords[10],true,int32_t(asset.frames));
  if(frame<0||uint32_t(frame)>=asset.frames)throw std::runtime_error("projected light frame out of range");
  auto selectedMip=std::min(mip,asset.mips-1);auto key=std::make_tuple(path,uint32_t(frame),selectedMip);auto image=images.find(key);
  if(image==images.end()){auto decoded=std::make_shared<const VtfRgbaImageCandidate>(decodeVtfMipCandidate(asset.bytes,selectedMip,uint32_t(frame)));image=images.emplace(key,std::move(decoded)).first;}
  return {path,uint32_t(frame),selectedMip,image->second};
 }
 size_t assetCount()const{return assets.size();}
 size_t imageCount()const{return images.size();}
};
