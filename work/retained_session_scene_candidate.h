#pragma once
#include "live_session_scene_candidate.h"
#include "prepared_session_scene_candidate.h"
#include "session_movie_dimensions_candidate.h"
#include "session_frame_rate_candidate.h"
#include "read_binary_asset_candidate.h"
#include "source_bsp_inline_brush_candidate.h"
#include "source_bsp_inline_model_name_candidate.h"
#include "retained_source_bsp_world_candidate.h"
#include "scene_hdr2_profile_candidate.h"
#include "session_model_illumination_dag_candidate.h"
#include "session_ifm_bone_inputs_candidate.h"
#include "ik_matrix_vector.h"
#pragma push_macro("near")
#pragma push_macro("far")
#undef near
#undef far
#include "source_new_record_leaf_ambient_candidate.h"
#include "source_bsp_light_cache_world_candidate.h"
#include "source_bsp_displacement_collision_candidate.h"
#include "source_bsp_world_light_trace_candidate.h"
#pragma pop_macro("far")
#pragma pop_macro("near")
#include <charconv>
using RetainedModelDeformationCandidate=std::function<VvdGeometryCandidate(const Graph&,const SessionRenderModelCandidate&,const SfmModelGeometryCandidate&)>;
using RetainedBorrowedDeformationCandidate=std::function<const VvdGeometryCandidate&(const Graph&,const SessionRenderModelCandidate&,const SfmModelGeometryCandidate&)>;
class RetainedSessionSceneCandidate {
 RetainedShotTimelineCandidate timeline_;
 std::vector<std::filesystem::path>mounts_;
 using AssetKey=std::pair<std::string,std::filesystem::path>;
 std::map<AssetKey,SfmModelGeometryCandidate>assets_;
 GeometryDeformationWorkerPoolCandidate workers_;
 SceneRenderInputsCacheCandidate materials_;
 const std::shared_ptr<const SceneHdr2ProfileCandidate>hdr2Profile_;
 RetainedSourceBspWorldCandidate world_;
 bool allowScale_,allowNameMapping_;
 size_t scalarWorkers_=2,vectorWorkers_=2,expressionWorkers_=0;
 SceneGeometryDecorationCandidate decoration_;RetainedModelDeformationCandidate deformation_;RetainedBorrowedDeformationCandidate borrowed_;
 // Mounts and dataset/profile are fixed for this owner. Resolved map identity
 // distinguishes inline names across sessions; old cache entries are retained.
 std::filesystem::path inlineMapPath()const{
  const auto&graph=timeline_.savedGraph();const auto&root=graph.elements.at(timeline_.renderRoot());const Attribute*mapAttribute=nullptr;for(const auto&a:root.attributes)if(graph.strings.at(a.name)=="mapname"){if(mapAttribute)throw std::runtime_error("duplicate session mapname");mapAttribute=&a;}if(!mapAttribute||mapAttribute->type!=5||mapAttribute->values.size()!=1)throw std::runtime_error("session mapname unavailable for BSP inline model");
  std::string map=mapAttribute->values[0].text;if(map.empty())throw std::runtime_error("empty session mapname");for(char&c:map)if(c=='\\')c='/';std::filesystem::path relative(map);if(relative.is_absolute()||relative.has_root_name()||relative.has_root_directory()||map.find(':')!=std::string::npos)throw std::runtime_error("unsafe session mapname");for(const auto&part:relative)if(part=="..")throw std::runtime_error("session map traversal");relative.replace_extension(".bsp");std::filesystem::path bspPath;for(const auto&mount:mounts_){auto candidate=mount/"maps"/relative;if(std::filesystem::is_regular_file(candidate)){bspPath=candidate;break;}}if(bspPath.empty())throw std::runtime_error("session BSP map missing: "+map);
  return std::filesystem::canonical(bspPath);
 }
 std::map<std::string,std::vector<uint8_t>> lightingModelBytes_;
 struct AmbientMap {
  SourceBspBrushOnlyTraceCandidate brush;
  SourceBspDisplacementCollisionCandidate displacements;
  SourcePreparedWorldRayCandidate rays;
  SourceBspLightCacheWorldCandidate world;
  SourceBspLeafAmbientCandidate hdr;
  SourceBspTraceSurfaceFlagsCandidate surfaces;
  SourceBspWorldLightTraceCandidate skyTrace;
  static std::vector<uint32_t> surfaceIds(const std::vector<uint8_t>& bytes){
   if(bytes.size()<1036)throw std::runtime_error("truncated BSP lighting header");
   uint32_t length;std::memcpy(&length,bytes.data()+8+6*16+4,4);
   if(length%72 || length/72>65535 || length>bytes.size())throw std::runtime_error("unsupported BSP texinfo extent");
   std::vector<uint32_t> ids(length/72);
   for(size_t n=0;n<ids.size();++n)ids[n]=static_cast<uint32_t>(n);
   return ids;
  }
  explicit AmbientMap(const std::vector<uint8_t>& bytes):AmbientMap(bytes,surfaceIds(bytes)){}
  AmbientMap(const std::vector<uint8_t>& bytes,const std::vector<uint32_t>& ids)
   :brush(bytes,ids),displacements(bytes),rays(brush,displacements.meshes),
    world(bytes,brush,rays),hdr(bytes,SourceBspAmbientEncodingCandidate::Hdr),surfaces(bytes,ids),skyTrace(rays,surfaces){}
  AmbientMap(const AmbientMap&)=delete;
 };
 std::unique_ptr<AmbientMap> ambientMap_;
 std::filesystem::path ambientMapPath_;
 void sampleModelAmbient(PreparedSessionSceneCandidate& frame){
  if(!frame.hdrSkyLights || (frame.explicitLightOrigins.empty() && frame.animatedDrawPositions.empty()))return;
  const auto path=inlineMapPath();
  if(path!=ambientMapPath_){
   std::unique_ptr<AmbientMap> next;
   try{next=std::make_unique<AmbientMap>(readBinaryAssetCandidate(path));}
   catch(const std::runtime_error& error){std::cout<<"HDR model ambient unavailable: "<<error.what()<<"\n";}
   ambientMap_=std::move(next);
   ambientMapPath_=path;
  }
  if(!ambientMap_)return;
  auto sample=[&](const auto& origins){
   for(const auto& [model,position]:origins){
    if(frame.modelAmbientCubes.count(model))continue;
    const auto result=sourceNewRecordLeafAmbientCandidate(position,1,{4,false,0,0},ambientMap_->world,ambientMap_->hdr);
    frame.modelAmbientCubes.emplace(model,result.cube);
    auto& visibility=frame.modelSkyVisible[model];
    for(const auto& light:*frame.hdrSkyLights){
     std::array<float,3> delta{};
     for(size_t axis=0;axis<3;++axis){
      const float end=result.samplePoint[axis]+light.towardLight[axis]*57016.3203125f;
      delta[axis]=end-result.samplePoint[axis];
     }
     const bool swept=sourceWorldLightSquaredCandidate(delta)!=0;
     const auto hit=ambientMap_->skyTrace.trace({result.samplePoint,delta,swept},0x4481u,{});
     visibility.push_back((hit.surfaceFlags&4)!=0);
    }
   }
  };
  sample(frame.explicitLightOrigins);
  sample(frame.animatedDrawPositions);
 }
 void prepareModelLightingInputs(const Graph& graph,const SessionRenderSceneCandidate& scene,
     const std::vector<SceneGeometryModelCandidate>& geometry,PreparedSessionSceneCandidate& prepared){
  const auto camera=readSessionCameraSettingsCandidate(graph,scene.camera);
  const auto cameraMatrix=graphLocalTransformMatrix(graph,camera.transform);
  for(unsigned i=0;i<3;++i)prepared.lightingCameraPosition[i]=cameraMatrix[i][3];
  std::set<uint32_t> members;
  const auto* sets=namedAttribute(graph,graph.elements.at(scene.shot),"animationSets");
  if(sets && sets->type==15){
   for(const auto& entry:sets->values){
    if(entry.reference>=graph.elements.size())continue;
    const auto& set=graph.elements[entry.reference];
    if(graph.strings.at(set.type)!="DmeAnimationSet")continue;
    const auto* model=namedAttribute(graph,set,"gameModel");
    if(model && model->type==1 && model->values.size()==1)
     members.insert(model->values.front().reference);
   }
  }
  std::optional<GraphTransformHierarchy> hierarchy;
  for(const auto& item:geometry){
   const auto id=item.model.id;
   prepared.lightingModelIds.push_back(id);
   if(!item.model.visible || graph.strings.at(graph.elements.at(id).type)!="DmeGameModel")continue;
   if(members.count(id))prepared.animatedBoundsModels.insert(id);
   const auto selection=sessionModelIlluminationDagCandidate(graph,id,[&](uint32_t ref){
    return graph.strings.at(graph.elements.at(ref).type)=="DmeDag";
   });
   if(!selection.dag){
    const auto source=lightingModelBytes_.find(item.model.placement.asset);
    if(selection.attributePresent || !members.count(id) || source==lightingModelBytes_.end())continue;
    const auto& element=graph.elements.at(id);
    const auto* children=namedAttribute(graph,element,"children");
    if(!children || children->type!=15 || children->values.empty())continue;
    const auto child=children->values.front().reference;
    if(child>=graph.elements.size() || graph.strings.at(graph.elements[child].type)!="DmeDag")continue;
    const auto& mdl=source->second;
    const auto hitboxes=readIfmMdlHitboxesCandidate(mdl);
    if(hitboxes.firstSet.empty())continue;
    const auto poses=sessionIfmBoneInputsCandidate(graph,id,mdl,[&](uint32_t ref){
     return graph.strings.at(graph.elements.at(ref).type)=="DmeTransform";
    });
    if(std::any_of(poses.begin(),poses.end(),[](const auto& bone){return (bone.flags&4)!=0;}))continue;
    if(!hierarchy)hierarchy.emplace(graph,readFilmSceneRenderBindingCandidate(graph,scene.shot).scene);
    const auto world=hierarchy->world(id);
    // This normal-palette path supports rigid, unit-scale models only.
    bool rigid=true;
    for(size_t a=0;a<3;++a)for(size_t b=a;b<3;++b){
     float dot=0;for(size_t row=0;row<3;++row)dot+=world[row][a]*world[row][b];
     if(std::abs(dot-(a==b?1.f:0.f))>0.00001f)rigid=false;
    }
    if(!rigid)continue;
    const auto palette=ifmBoneWorldPaletteCandidate(poses,world,0x7ff00);
    const auto bounds=ifmAnimatedHitboxBoundsCandidate(&hitboxes.firstSet,false,0,
     [&](auto& output,uint32_t,uint32_t,float){
      for(const auto& box:hitboxes.firstSet)output.at(box.bone)=palette.at(box.bone).value();
     },[&]{return world;},[]{return 1.f;});
    std::array<float,3> center{};
    for(size_t axis=0;axis<3;++axis)center[axis]=((*bounds)[0][axis]+(*bounds)[1][axis])*.5f;
    prepared.animatedDrawPositions.emplace(id,transformRecoveredIkPointCandidate(center,world));
    continue;
   }
   if(!hierarchy)hierarchy.emplace(graph,readFilmSceneRenderBindingCandidate(graph,scene.shot).scene);
   const auto matrix=hierarchy->world(*selection.dag);
   std::array<float,3> point{matrix[0][3],matrix[1][3],matrix[2][3]};
   for(float value:point)if(!std::isfinite(value))throw std::runtime_error("nonfinite model illumination DAG position");
   prepared.explicitLightOrigins.emplace(id,point);
  }
  sampleModelAmbient(prepared);
 }
 SfmModelGeometryCandidate loadInlineBrush(const std::string&name,const std::filesystem::path&bspPath){
  const size_t index=sourceBspInlineModelIndexCandidate(name);
  auto brush=readSourceBspInlineBrushCandidate(readBinaryAssetCandidate(bspPath),index,nullptr,hdr2Profile_?hdr2Profile_->dataset():SourceBspLightmapSelectionCandidate::LegacyPreview);return sourceBspStaticModelCandidate(brush);
 }
 const SfmModelGeometryCandidate&asset(const std::string&name){
  const AssetKey key{name,name.size()>1&&name[0]=='*'?inlineMapPath():std::filesystem::path{}};
  auto found=assets_.find(key);if(found!=assets_.end())return found->second;
  if(name.size()>1&&name[0]=='*')return assets_.emplace(key,loadInlineBrush(name,key.second)).first->second;
  std::string portable=name;for(char&c:portable)if(c=='\\')c='/';std::filesystem::path relative(portable);
  if(relative.empty()||relative.is_absolute()||relative.has_root_name()||relative.has_root_directory()||portable.find(':')!=std::string::npos)throw std::runtime_error("scene asset relative path");for(const auto&part:relative)if(part=="..")throw std::runtime_error("scene asset traversal");
  std::filesystem::path path;for(const auto&mount:mounts_)if(std::filesystem::is_regular_file(mount/relative)){path=mount/relative;break;}if(path.empty())throw std::runtime_error("scene model missing: "+name);path.replace_extension();
  auto read=[](const std::filesystem::path&file){try{return readBinaryAssetCandidate(file);}catch(const std::exception&error){throw std::runtime_error("scene model companion read failed: "+file.string()+": "+error.what());}};
  auto mdl=read(path.string()+".mdl");
  auto geometry=readSfmModelGeometryCandidate(mdl,read(path.string()+".vvd"),read(path.string()+".dx90.vtx"));
  if(hdr2Profile_)lightingModelBytes_.emplace(name,std::move(mdl));
  return assets_.emplace(key,std::move(geometry)).first->second;
 }
 // Native CreatePuppets does not create a puppet for an unavailable inline
 // name without an effects attribute. Preserve its document node/children;
 // omit only that render-model entry and surface a diagnostic. All corrupt
 // BSP errors and the unimplemented effects fallback remain explicit errors.
 std::vector<SceneUnavailableModelDiagnosticCandidate>admitInlineModels(SessionRenderSceneCandidate&scene){
  std::vector<SceneUnavailableModelDiagnosticCandidate>missing;
  auto keep=scene.models.begin();
  for(auto it=scene.models.begin();it!=scene.models.end();++it){
   bool admitted=true;const auto&name=it->placement.asset;
   if(name.size()>1&&name[0]=='*')try{(void)asset(name);}
   catch(const SourceBspModelIndexUnavailableCandidate&error){
    if(!error.count)throw;
    if(namedAttribute(timeline_.graph(),timeline_.graph().elements.at(it->id),"effects"))
     throw std::runtime_error("unavailable inline model effects fallback unsupported: "+name);
    missing.push_back({it->id,name,error.index,error.count});admitted=false;
    std::cout<<"unavailable_inline_model="<<name<<" element="<<it->id<<" draw_omitted=1 document_preserved=1\n";
   }
   if(admitted){if(keep!=it)*keep=std::move(*it);++keep;}
  }
  scene.models.erase(keep,scene.models.end());return missing;
 }
public:
 // Caller finishes outstanding sampling before changing worker configuration.
 void setTimelineWorkersCandidate(size_t scalar,size_t vector,size_t expression){
  if(scalar>64||vector>64||expression>64||scalar+vector+expression>64)throw std::runtime_error("combined sampling worker budget exceeds 64");
  scalarWorkers_=scalar;vectorWorkers_=vector;expressionWorkers_=expression;
 }
 // Caller must finish outstanding sampling before replacing this provider.
 void setSceneDecorationCandidate(SceneGeometryDecorationCandidate decoration){decoration_=std::move(decoration);}
 void addSceneDecorationCandidate(SceneGeometryDecorationCandidate decoration){auto previous=decoration_;decoration_=[previous,decoration](const auto& graph,const auto& scene,const auto& geometry,const auto& asset,auto& prepared){if(previous)previous(graph,scene,geometry,asset,prepared);decoration(graph,scene,geometry,asset,prepared);};}
 void setModelDeformationCandidate(RetainedModelDeformationCandidate provider){deformation_=std::move(provider);borrowed_={};}
 void setBorrowedDeformationCandidate(RetainedBorrowedDeformationCandidate provider){borrowed_=std::move(provider);deformation_={};}
 RetainedSessionSceneCandidate(Graph graph,uint32_t root,std::vector<std::filesystem::path>mounts,size_t geometryWorkers=4,bool allowScale=false,bool allowNameMapping=false,std::shared_ptr<const SceneHdr2ProfileCandidate>hdr2Profile={}):timeline_(std::move(graph),root),mounts_(std::move(mounts)),workers_(geometryWorkers),materials_(mounts_,2,[this](const std::string&name)->const SfmModelGeometryCandidate&{return asset(name);},[this](const std::string&name){return name.size()>1&&name[0]=='*'?inlineMapPath():std::filesystem::path{};},bool(hdr2Profile)),hdr2Profile_(std::move(hdr2Profile)),world_(mounts_,hdr2Profile_?hdr2Profile_->dataset():SourceBspLightmapSelectionCandidate::LegacyPreview),allowScale_(allowScale),allowNameMapping_(allowNameMapping){}
 RetainedSessionSceneCandidate(const RetainedSessionSceneCandidate&)=delete;
 RetainedSessionSceneCandidate&operator=(const RetainedSessionSceneCandidate&)=delete;
 // Replace timeline state while retaining immutable assets/material inputs.
 // Caller must finish any outstanding sample before replacing the session.
 void replaceSession(Graph graph,uint32_t root){
  static_assert(std::is_nothrow_move_assignable<RetainedShotTimelineCandidate>::value,"timeline commit must not throw");
  RetainedShotTimelineCandidate next(std::move(graph),root);timeline_=std::move(next);
 }
 PreparedSessionSceneCandidate sample(int32_t tick,uint32_t width=512,uint32_t height=512){auto start=std::chrono::steady_clock::now();auto step=evaluateLiveSessionSceneCandidate(timeline_,tick,width,height,scalarWorkers_,vectorWorkers_,expressionWorkers_);auto evaluated=std::chrono::steady_clock::now();auto unavailableModels=admitInlineModels(step.scene);auto admittedAt=std::chrono::steady_clock::now();if(expressionWorkers_)std::cout<<"scene_private_expression_jobs="<<step.evaluation.privateExpressionJobs<<"\n";SceneModelDeformationCandidate deform;if(deformation_)deform=[this](const SessionRenderModelCandidate&instance,const SfmModelGeometryCandidate&model){return deformation_(timeline_.graph(),instance,model);};SceneBorrowedDeformationCandidate borrowed;if(borrowed_)borrowed=[this](const SessionRenderModelCandidate&instance,const SfmModelGeometryCandidate&model)->const VvdGeometryCandidate&{return borrowed_(timeline_.graph(),instance,model);};auto decorate=[this](const Graph&graph,const SessionRenderSceneCandidate&scene,const std::vector<SceneGeometryModelCandidate>&geometry,const auto&assetLoader,PreparedSessionSceneCandidate&prepared){world_.append(graph,timeline_.renderRoot(),prepared);if(hdr2Profile_)prepareModelLightingInputs(graph,scene,geometry,prepared);if(decoration_)decoration_(graph,scene,geometry,assetLoader,prepared);};auto result=prepareSessionSceneCandidate(timeline_.graph(),step.scene,[this](const std::string&name)->const SfmModelGeometryCandidate&{return asset(name);},materials_,workers_,mounts_,allowScale_,allowNameMapping_,deform,borrowed,decorate);result.profile.evaluation=std::chrono::duration<double,std::milli>(evaluated-start).count();result.profile.poses+=std::chrono::duration<double,std::milli>(admittedAt-evaluated).count();result.missingModels=std::move(unavailableModels);
  if(hdr2Profile_){
   if(hdr2Profile_->usesSessionExposure())result.sessionExposure=readSessionExposureCandidate(timeline_.graph(),timeline_.renderRoot(),step.scene.camera);
   result.hdr2=SceneHdr2PreparedFrameCandidate::prepare(result,hdr2Profile_);
   if(result.sessionExposure){const auto& exposure=*result.sessionExposure;std::cout<<"session_camera_exposure=1 tick="<<tick<<" camera="<<exposure.camera<<" session_scale="<<exposure.sessionScale<<" camera_scale="<<exposure.cameraScale<<" product="<<exposure.product<<" scope=admitted_lightmaps_only\n";}
  }
  return result;}
 size_t assetCount()const{return assets_.size();}size_t materialCount()const{return materials_.size();}size_t materialHits()const{return materials_.hits();}
 PreparedSessionSceneCandidate samplePlayback(int32_t tick,uint32_t width,uint32_t height){auto rate=sessionFrameRateCandidate(timeline_.savedGraph(),timeline_.renderRoot(),false);auto frame=sample(tick,width,height);frame.packedFrameRate=rate;return frame;}
 // Explicit prototype blank-frame policy; native gap display remains unverified.
 PreparedSessionSceneCandidate sampleTimeline(int32_t tick,uint32_t width,uint32_t height,FilmTimelineStateCandidate&state){
  if(!width||!height)throw std::runtime_error("timeline dimensions must be positive");auto selection=timeline_.classifyFrame(tick);state=selection.state;
  if(state==FilmTimelineStateCandidate::SceneClip)return samplePlayback(tick,width,height);
  PreparedSessionSceneCandidate blank{};blank.outputWidth=width;blank.outputHeight=height;blank.packedFrameRate=sessionFrameRateCandidate(timeline_.savedGraph(),timeline_.renderRoot(),false);if(hdr2Profile_)blank.hdr2=SceneHdr2PreparedFrameCandidate::prepare(blank,hdr2Profile_);return blank;
 }
 PreparedSessionSceneCandidate sampleMovie(int32_t tick){auto dimensions=sessionMovieDimensionsCandidate(timeline_.savedGraph(),timeline_.renderRoot());return samplePlayback(tick,dimensions.width,dimensions.height);}
 PreparedSessionSceneCandidate sampleTimelineOutput(int32_t tick,uint32_t width,uint32_t height,bool movie=false){if(movie){auto dimensions=sessionMovieDimensionsCandidate(timeline_.savedGraph(),timeline_.renderRoot());width=dimensions.width;height=dimensions.height;}FilmTimelineStateCandidate state;auto result=sampleTimeline(tick,width,height,state);std::cout<<"timeline_frame_state="<<int(state)<<" tick="<<tick<<" native_blank_policy=unverified\n";return result;}
};

