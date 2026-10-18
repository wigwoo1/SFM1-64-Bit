#pragma once
#include "source_bsp_inline_brush_candidate.h"
#include "model_render_inputs_candidate.h"
#include "scene_gpu_upload_candidate.h"
#include "read_binary_asset_candidate.h"
#include "render_camera_file_candidate.h"
#include <memory>

// Static Source BSP world geometry appended to a prepared session scene. The
// parsed model/materials are retained across animation samples.
class RetainedSourceBspWorldCandidate {
 std::vector<std::filesystem::path> mounts_;
 std::filesystem::path loadedPath_;
 SfmModelGeometryCandidate model_;
 std::vector<std::array<float,3>> lightColors_;
 std::vector<float> displacementAlphas_;
 std::shared_ptr<const ModelRenderInputsCandidate> inputs_;
 static std::string mapName(const Graph&graph,uint32_t root){
  if(root>=graph.elements.size())return {};
  for(const auto&a:graph.elements[root].attributes)if(graph.strings.at(a.name)=="mapname"){
   if(a.type!=5||a.values.size()!=1)throw std::runtime_error("session mapname format");return a.values[0].text;
  }
  return {};
 }
 void load(const std::string&name){
  std::string portable=name;for(char&c:portable)if(c=='\\')c='/';std::filesystem::path relative(portable);
  if(relative.empty()||relative.is_absolute()||relative.has_root_name()||relative.has_root_directory()||portable.find(':')!=std::string::npos)throw std::runtime_error("unsafe session mapname");
  for(const auto&part:relative)if(part=="..")throw std::runtime_error("session map traversal");relative.replace_extension(".bsp");
  std::filesystem::path path;for(const auto&mount:mounts_){auto candidate=mount/"maps"/relative;if(std::filesystem::is_regular_file(candidate)){path=candidate;break;}}
  if(path.empty())throw std::runtime_error("session BSP map missing: "+name);
  auto bytes=readBinaryAssetCandidate(path);auto world=readSourceBspInlineBrushCandidate(bytes,0);auto nextModel=sourceBspStaticModelCandidate(world);auto nextInputs=std::make_shared<ModelRenderInputsCandidate>(prepareModelRenderInputsCandidate(nextModel,0,mounts_,0));for(auto&material:nextInputs->materials)material.vertexLighting=true;
  model_=std::move(nextModel);lightColors_=std::move(world.lightColors);displacementAlphas_=std::move(world.displacementAlphas);inputs_=std::move(nextInputs);loadedPath_=path;
 }
public:
 explicit RetainedSourceBspWorldCandidate(std::vector<std::filesystem::path>mounts):mounts_(std::move(mounts)){}
 void append(const Graph&graph,uint32_t root,PreparedSessionSceneCandidate&prepared){
  auto name=mapName(graph,root);if(name.empty())return;
  std::string normalized=name;for(char&c:normalized)if(c=='\\')c='/';std::filesystem::path relative(normalized);relative.replace_extension(".bsp");std::filesystem::path path;for(const auto&mount:mounts_){auto candidate=mount/"maps"/relative;if(std::filesystem::is_regular_file(candidate)){path=candidate;break;}}
  if(path.empty())throw std::runtime_error("session BSP map missing: "+name);if(path!=loadedPath_)load(name);
  if(!inputs_||model_.geometry.vertices.empty())throw std::runtime_error("empty BSP world model");
  auto&upload=prepared.upload;auto&materialInputs=prepared.materialInputs;
  const size_t vertexBase=upload.vertices.size(),indexBase=upload.indices.size(),slotBase=upload.materials.size(),opaqueBase=upload.opaque.size(),translucentBase=upload.translucent.size(),alphaBase=prepared.materialAlpha.size();
  const size_t entriesBase=materialInputs.entries.size(),ownersBase=materialInputs.owners.size(),inputAlphaBase=materialInputs.alpha.size();
  if(vertexBase>UINT32_MAX||model_.geometry.vertices.size()>UINT32_MAX-vertexBase||indexBase>UINT32_MAX||inputs_->indices.size()>UINT32_MAX-indexBase||slotBase>UINT32_MAX||inputs_->materials.size()>UINT32_MAX-slotBase)throw std::runtime_error("BSP world GPU buffer overflow");
  try{
   upload.vertices.reserve(vertexBase+model_.geometry.vertices.size());upload.indices.reserve(indexBase+inputs_->indices.size());upload.materials.reserve(slotBase+inputs_->materials.size());upload.opaque.reserve(opaqueBase+inputs_->ranges.size());upload.translucent.reserve(translucentBase+inputs_->ranges.size());prepared.materialAlpha.reserve(alphaBase+inputs_->materials.size());
   const size_t group=materialInputs.appendOwned(inputs_);
   for(size_t material=0;material<inputs_->materials.size();++material){upload.materials.push_back({group,static_cast<uint32_t>(material)});prepared.materialAlpha.push_back(1.f);}
   if(lightColors_.size()!=model_.geometry.vertices.size()||displacementAlphas_.size()!=model_.geometry.vertices.size())throw std::runtime_error("BSP world vertex channel coverage");for(size_t i=0;i<model_.geometry.vertices.size();++i){const auto&raw=model_.geometry.vertices[i];SceneGpuVertexCandidate vertex;std::memcpy(vertex.position.data(),raw.data()+16,12);vertex.normal=lightColors_[i];std::memcpy(vertex.uv.data(),raw.data()+40,8);vertex.displacementAlpha=displacementAlphas_[i];upload.vertices.push_back(vertex);}
   for(auto index:inputs_->indices){if(index>=model_.geometry.vertices.size())throw std::runtime_error("BSP world vertex index");upload.indices.push_back(static_cast<uint32_t>(vertexBase)+index);}
   std::vector<SceneGpuDrawCandidate>worldTranslucent;
   for(size_t rangeIndex=0;rangeIndex<inputs_->ranges.size();++rangeIndex){const auto&r=inputs_->ranges[rangeIndex];if(r.materialIndex>=inputs_->materials.size()||r.firstIndex>inputs_->indices.size()||r.indexCount>inputs_->indices.size()-r.firstIndex||r.indexCount%3)throw std::runtime_error("BSP world material range");if(!r.indexCount)continue;SceneGpuDrawCandidate draw{group,rangeIndex,static_cast<uint32_t>(indexBase+r.firstIndex),r.indexCount,r.materialIndex,static_cast<uint32_t>(slotBase+r.materialIndex)};if(inputs_->materials[r.materialIndex].translucent)worldTranslucent.push_back(draw);else upload.opaque.push_back(draw);}
   // World transparency is ordered by the mean camera-space depth of each
   // range, matching the prototype's painter-order policy for other models.
   std::vector<std::pair<double,SceneGpuDrawCandidate>>depths;depths.reserve(worldTranslucent.size());
   for(const auto&draw:worldTranslucent){double depth=0;for(uint32_t n=0;n<draw.indexCount;++n){const auto&vertex=upload.vertices.at(upload.indices.at(size_t(draw.firstIndex)+n));depth+=renderCameraSortDistanceCandidate(prepared.camera,vertex.position);}depth/=draw.indexCount;if(!std::isfinite(depth))throw std::runtime_error("BSP world translucent depth");depths.push_back({depth,draw});}
   std::stable_sort(depths.begin(),depths.end(),[](const auto&a,const auto&b){return a.first>b.first;});for(auto&entry:depths)upload.translucent.push_back(entry.second);
  }catch(...){upload.vertices.resize(vertexBase);upload.indices.resize(indexBase);upload.materials.resize(slotBase);upload.opaque.resize(opaqueBase);upload.translucent.resize(translucentBase);prepared.materialAlpha.resize(alphaBase);materialInputs.entries.resize(entriesBase);materialInputs.owners.resize(ownersBase);materialInputs.alpha.resize(inputAlphaBase);throw;}
 }
};
