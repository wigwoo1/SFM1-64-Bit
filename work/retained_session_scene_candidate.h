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
  return assets_.emplace(key,readSfmModelGeometryCandidate(read(path.string()+".mdl"),read(path.string()+".vvd"),read(path.string()+".dx90.vtx"))).first->second;
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
 RetainedSessionSceneCandidate(Graph graph,uint32_t root,std::vector<std::filesystem::path>mounts,size_t geometryWorkers=4,bool allowScale=false,bool allowNameMapping=false,std::shared_ptr<const SceneHdr2ProfileCandidate>hdr2Profile={}):timeline_(std::move(graph),root),mounts_(std::move(mounts)),workers_(geometryWorkers),materials_(mounts_,2,[this](const std::string&name)->const SfmModelGeometryCandidate&{return asset(name);},[this](const std::string&name){return name.size()>1&&name[0]=='*'?inlineMapPath():std::filesystem::path{};}),hdr2Profile_(std::move(hdr2Profile)),world_(mounts_,hdr2Profile_?hdr2Profile_->dataset():SourceBspLightmapSelectionCandidate::LegacyPreview),allowScale_(allowScale),allowNameMapping_(allowNameMapping){}
 RetainedSessionSceneCandidate(const RetainedSessionSceneCandidate&)=delete;
 RetainedSessionSceneCandidate&operator=(const RetainedSessionSceneCandidate&)=delete;
 // Replace timeline state while retaining immutable assets/material inputs.
 // Caller must finish any outstanding sample before replacing the session.
 void replaceSession(Graph graph,uint32_t root){
  static_assert(std::is_nothrow_move_assignable<RetainedShotTimelineCandidate>::value,"timeline commit must not throw");
  RetainedShotTimelineCandidate next(std::move(graph),root);timeline_=std::move(next);
 }
 PreparedSessionSceneCandidate sample(int32_t tick,uint32_t width=512,uint32_t height=512){auto start=std::chrono::steady_clock::now();auto step=evaluateLiveSessionSceneCandidate(timeline_,tick,width,height,scalarWorkers_,vectorWorkers_,expressionWorkers_);auto evaluated=std::chrono::steady_clock::now();auto unavailableModels=admitInlineModels(step.scene);auto admittedAt=std::chrono::steady_clock::now();if(expressionWorkers_)std::cout<<"scene_private_expression_jobs="<<step.evaluation.privateExpressionJobs<<"\n";SceneModelDeformationCandidate deform;if(deformation_)deform=[this](const SessionRenderModelCandidate&instance,const SfmModelGeometryCandidate&model){return deformation_(timeline_.graph(),instance,model);};SceneBorrowedDeformationCandidate borrowed;if(borrowed_)borrowed=[this](const SessionRenderModelCandidate&instance,const SfmModelGeometryCandidate&model)->const VvdGeometryCandidate&{return borrowed_(timeline_.graph(),instance,model);};auto decorate=[this](const Graph&graph,const SessionRenderSceneCandidate&scene,const std::vector<SceneGeometryModelCandidate>&geometry,const auto&assetLoader,PreparedSessionSceneCandidate&prepared){world_.append(graph,timeline_.renderRoot(),prepared);if(decoration_)decoration_(graph,scene,geometry,assetLoader,prepared);};auto result=prepareSessionSceneCandidate(timeline_.graph(),step.scene,[this](const std::string&name)->const SfmModelGeometryCandidate&{return asset(name);},materials_,workers_,mounts_,allowScale_,allowNameMapping_,deform,borrowed,decorate);result.profile.evaluation=std::chrono::duration<double,std::milli>(evaluated-start).count();result.profile.poses+=std::chrono::duration<double,std::milli>(admittedAt-evaluated).count();result.missingModels=std::move(unavailableModels);if(hdr2Profile_)result.hdr2=SceneHdr2PreparedFrameCandidate::prepare(result,hdr2Profile_);return result;}
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

