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
 // Fixed through the public lifetime of this loader; never inferred from the
 // presence of lighting lumps or a renderer's current hardware configuration.
 SourceBspLightmapSelectionCandidate selection_;
 struct Cache {
  std::filesystem::path path;
  SourceBspLightmapSelectionCandidate selection=SourceBspLightmapSelectionCandidate::LegacyPreview;
  std::shared_ptr<const std::vector<SourceSkyLightCandidate>> skyLights;
  SfmModelGeometryCandidate model;
  std::vector<std::array<float,3>>lightColors;
  std::vector<float>displacementAlphas;
  std::shared_ptr<const ModelRenderInputsCandidate>inputs;
  std::shared_ptr<const SourceBspLightmapSnapshotCandidate>lightmaps;
 };
 std::shared_ptr<const Cache>cache_;
 using Hook=std::function<void(const char*)>;
 static void stage(const Hook&hook,const char*name){if(hook)hook(name);}
 static std::string mapName(const Graph&graph,uint32_t root){
  if(root>=graph.elements.size())return {};
  for(const auto&a:graph.elements[root].attributes)if(graph.strings.at(a.name)=="mapname"){
   if(a.type!=5||a.values.size()!=1)throw std::runtime_error("session mapname format");return a.values[0].text;
  }
  return {};
 }
 std::shared_ptr<const Cache> load(const std::string&name,const Hook&hook){
  std::string portable=name;for(char&c:portable)if(c=='\\')c='/';std::filesystem::path relative(portable);
  if(relative.empty()||relative.is_absolute()||relative.has_root_name()||relative.has_root_directory()||portable.find(':')!=std::string::npos)throw std::runtime_error("unsafe session mapname");
  for(const auto&part:relative)if(part=="..")throw std::runtime_error("session map traversal");relative.replace_extension(".bsp");
  std::filesystem::path path;for(const auto&mount:mounts_){auto candidate=mount/"maps"/relative;if(std::filesystem::is_regular_file(candidate)){path=candidate;break;}}
  if(path.empty())throw std::runtime_error("session BSP map missing: "+name);
  auto bytes=readBinaryAssetCandidate(path);stage(hook,"load-read");
  SourceBspLightmappedBrushCandidate world;bool available=false;std::string unavailable;
  try{world=readSourceBspLightmappedBrushCandidate(bytes,0,selection_);available=true;}
  catch(const std::runtime_error&error){if(selection_!=SourceBspLightmapSelectionCandidate::LegacyPreview)throw;unavailable=error.what();world.geometry=readSourceBspInlineBrushCandidate(bytes,0);}
  stage(hook,"load-geometry");
  Cache next;
  if(selection_==SourceBspLightmapSelectionCandidate::Hdr){
   auto sky=readSourceHdrSkyLightsCandidate(bytes);
   if(sky)next.skyLights=std::make_shared<const std::vector<SourceSkyLightCandidate>>(std::move(*sky));
  }
  next.path=path;next.selection=selection_;stage(hook,"load-path");next.model=sourceBspStaticModelCandidate(world.geometry);stage(hook,"load-model");
  auto nextInputs=std::make_shared<ModelRenderInputsCandidate>(prepareModelRenderInputsCandidate(next.model,0,mounts_,0));for(auto& material:nextInputs->materials){
   material.vertexLighting=true;
   const auto& syntax=material.source.syntax;
   if(selection_!=SourceBspLightmapSelectionCandidate::Hdr || vmtAsciiLowerCandidate(syntax.key)!="lightmappedgeneric" ||
      vmtValuesCandidate(syntax,"$ssbump")!=std::vector<std::string>{"1"})continue;
   auto names=vmtValuesCandidate(syntax,"$bumpmap");
   if(names.size()!=1 || names.front().empty())continue;
   try{
    const auto file=findMountedMaterialCandidate(mounts_,materialRelativePathCandidate(names.front(),".vtf"));
    if(file.empty())throw std::runtime_error("missing VTF");
    material.selfShadowedBump=prepareModelTextureSnapshotCandidate(names.front(),readBinaryAssetCandidate(file),0,0);
    if(!material.selfShadowedBump.cpu)throw std::runtime_error("unsupported VTF");
   }catch(const std::runtime_error& error){
    material.source.missingTextureReferences.push_back("$bumpmap "+names.front()+" ("+error.what()+")");
   }
  }stage(hook,"load-inputs");
  next.lightmaps=available?prepareSourceBspLightmapSnapshotCandidate(std::move(world),nextInputs->indices):unavailableSourceBspLightmapSnapshotCandidate(world.geometry.positions.size(),nextInputs->indices,std::move(unavailable));stage(hook,"load-snapshot");
  next.lightColors=std::move(world.geometry.lightColors);next.displacementAlphas=std::move(world.geometry.displacementAlphas);next.inputs=std::move(nextInputs);
  auto result=std::make_shared<const Cache>(std::move(next));stage(hook,"load-cache-ready");return result;
 }
public:
 explicit RetainedSourceBspWorldCandidate(std::vector<std::filesystem::path>mounts,SourceBspLightmapSelectionCandidate selection=SourceBspLightmapSelectionCandidate::LegacyPreview):mounts_(std::move(mounts)),selection_(selection){
  if(selection!=SourceBspLightmapSelectionCandidate::Ldr&&selection!=SourceBspLightmapSelectionCandidate::Hdr&&selection!=SourceBspLightmapSelectionCandidate::LegacyPreview)throw std::runtime_error("invalid retained BSP dataset selection");
 }
 void append(const Graph&graph,uint32_t root,PreparedSessionSceneCandidate&prepared,const Hook&hook={}){
  auto name=mapName(graph,root);if(name.empty())return;
  std::string normalized=name;for(char&c:normalized)if(c=='\\')c='/';std::filesystem::path relative(normalized);relative.replace_extension(".bsp");std::filesystem::path path;for(const auto&mount:mounts_){auto candidate=mount/"maps"/relative;if(std::filesystem::is_regular_file(candidate)){path=candidate;break;}}
  if(path.empty())throw std::runtime_error("session BSP map missing: "+name);
  auto retained=cache_;if(!retained||path!=retained->path||selection_!=retained->selection)retained=load(name,hook);
  const auto&model_=retained->model;const auto&lightColors_=retained->lightColors;const auto&displacementAlphas_=retained->displacementAlphas;const auto&inputs_=retained->inputs;
  if(!inputs_||model_.geometry.vertices.empty())throw std::runtime_error("empty BSP world model");
  auto&upload=prepared.upload;auto&materialInputs=prepared.materialInputs;
  const size_t vertexBase=upload.vertices.size(),indexBase=upload.indices.size(),slotBase=upload.materials.size(),opaqueBase=upload.opaque.size(),translucentBase=upload.translucent.size(),alphaBase=prepared.materialAlpha.size();
  const size_t entriesBase=materialInputs.entries.size(),ownersBase=materialInputs.owners.size(),inputAlphaBase=materialInputs.alpha.size();
  validateSceneShadingCandidate(upload);validateSceneBspLightmapsCandidate(upload);const size_t shadingBase=upload.vertexShading.size(),lightmapBase=upload.bspLightmaps.size();
  if(vertexBase>UINT32_MAX||model_.geometry.vertices.size()>UINT32_MAX-vertexBase||indexBase>UINT32_MAX||inputs_->indices.size()>UINT32_MAX-indexBase||slotBase>UINT32_MAX||inputs_->materials.size()>UINT32_MAX-slotBase)throw std::runtime_error("BSP world GPU buffer overflow");
  try{
   upload.bspLightmaps.reserve(lightmapBase+1);
   upload.vertexShading.reserve(vertexBase+model_.geometry.vertices.size());
   if(upload.vertexShading.empty())upload.vertexShading.resize(vertexBase);
   upload.vertices.reserve(vertexBase+model_.geometry.vertices.size());upload.indices.reserve(indexBase+inputs_->indices.size());upload.materials.reserve(slotBase+inputs_->materials.size());upload.opaque.reserve(opaqueBase+inputs_->ranges.size());upload.translucent.reserve(translucentBase+inputs_->ranges.size());prepared.materialAlpha.reserve(alphaBase+inputs_->materials.size());
   stage(hook,"append-reserved");
   const size_t group=materialInputs.appendOwned(inputs_);
   for(size_t material=0;material<inputs_->materials.size();++material){upload.materials.push_back({group,static_cast<uint32_t>(material)});prepared.materialAlpha.push_back(1.f);}
   stage(hook,"append-materials");
   if(lightColors_.size()!=model_.geometry.vertices.size()||displacementAlphas_.size()!=model_.geometry.vertices.size())throw std::runtime_error("BSP world vertex channel coverage");for(size_t i=0;i<model_.geometry.vertices.size();++i){const auto&raw=model_.geometry.vertices[i];SceneGpuVertexCandidate vertex;std::memcpy(vertex.position.data(),raw.data()+16,12);vertex.normal=lightColors_[i];std::memcpy(vertex.uv.data(),raw.data()+40,8);vertex.displacementAlpha=displacementAlphas_[i];SceneVertexShadingCandidate shading;std::memcpy(shading.normal.data(),raw.data()+28,12);shading.previewLightColor=lightColors_[i];shading.hasNormal=shading.hasPreviewLightColor=true;shading.origin=SceneShadingOriginCandidate::WorldGeometry;shading.model=group;shading.sourceVertex=static_cast<uint32_t>(i);validateSceneVertexShadingCandidate(shading);upload.vertices.push_back(vertex);upload.vertexShading.push_back(shading);}
   for(auto index:inputs_->indices){if(index>=model_.geometry.vertices.size())throw std::runtime_error("BSP world vertex index");upload.indices.push_back(static_cast<uint32_t>(vertexBase)+index);}
   stage(hook,"append-indices");
   std::vector<SceneGpuDrawCandidate>worldTranslucent;
   for(size_t rangeIndex=0;rangeIndex<inputs_->ranges.size();++rangeIndex){const auto&r=inputs_->ranges[rangeIndex];if(r.materialIndex>=inputs_->materials.size()||r.firstIndex>inputs_->indices.size()||r.indexCount>inputs_->indices.size()-r.firstIndex||r.indexCount%3)throw std::runtime_error("BSP world material range");if(!r.indexCount)continue;SceneGpuDrawCandidate draw{group,rangeIndex,static_cast<uint32_t>(indexBase+r.firstIndex),r.indexCount,r.materialIndex,static_cast<uint32_t>(slotBase+r.materialIndex)};if(inputs_->materials[r.materialIndex].translucent)worldTranslucent.push_back(draw);else upload.opaque.push_back(draw);}
   // World transparency is ordered by the mean camera-space depth of each
   // range, matching the prototype's painter-order policy for other models.
   std::vector<std::pair<double,SceneGpuDrawCandidate>>depths;depths.reserve(worldTranslucent.size());
   for(const auto&draw:worldTranslucent){double depth=0;for(uint32_t n=0;n<draw.indexCount;++n){const auto&vertex=upload.vertices.at(upload.indices.at(size_t(draw.firstIndex)+n));depth+=renderCameraSortDistanceCandidate(prepared.camera,vertex.position);}depth/=draw.indexCount;if(!std::isfinite(depth))throw std::runtime_error("BSP world translucent depth");depths.push_back({depth,draw});}
   std::stable_sort(depths.begin(),depths.end(),[](const auto&a,const auto&b){return a.first>b.first;});for(auto&entry:depths)upload.translucent.push_back(entry.second);
   stage(hook,"append-sorted");
   upload.bspLightmaps.push_back({group,static_cast<uint32_t>(vertexBase),static_cast<uint32_t>(model_.geometry.vertices.size()),static_cast<uint32_t>(indexBase),static_cast<uint32_t>(inputs_->indices.size()),retained->lightmaps});stage(hook,"append-associated");
   validateSceneShadingCandidate(upload);
   validateSceneBspLightmapsCandidate(upload);stage(hook,"append-ready-to-commit");prepared.hdrSkyLights=retained->skyLights;cache_.swap(retained);
  }catch(...){upload.bspLightmaps.resize(lightmapBase);upload.vertexShading.resize(shadingBase);upload.vertices.resize(vertexBase);upload.indices.resize(indexBase);upload.materials.resize(slotBase);upload.opaque.resize(opaqueBase);upload.translucent.resize(translucentBase);prepared.materialAlpha.resize(alphaBase);materialInputs.entries.resize(entriesBase);materialInputs.owners.resize(ownersBase);materialInputs.alpha.resize(inputAlphaBase);throw;}
 }
};
