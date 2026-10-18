#pragma once
#include "scene_geometry_preparation_candidate.h"
#include "scene_material_draw_inputs_candidate.h"
#include "scene_gpu_upload_candidate.h"
#include "render_camera_file_candidate.h"
#include <chrono>
struct ScenePreparationProfileCandidate {double evaluation=0,poses=0,geometry=0,materials=0,upload=0,decoration=0,furGuides=0,furRibbons=0,furAppend=0;};
struct SceneMissingAssetDiagnosticCandidate {std::string modelAsset;std::filesystem::path vmt;std::vector<std::string>references;};
inline std::string sceneMissingAssetSummaryCandidate(const std::vector<SceneMissingAssetDiagnosticCandidate>&diagnostics){
 size_t references=0;for(const auto&entry:diagnostics)references+=entry.references.size();if(!references)return {};
 const auto&first=diagnostics.front();return "Missing texture assets: "+std::to_string(references)+" | "+first.references.front();
}
struct PreparedSessionSceneCandidate {
 RenderCameraCandidate camera;
 SceneMaterialInputsCandidate materialInputs;
 SceneGpuUploadCandidate upload;
 std::vector<float>materialAlpha;
 uint32_t outputWidth=512,outputHeight=512;
 uint32_t packedFrameRate=0;
 ScenePreparationProfileCandidate profile;
 uint32_t shot=UINT32_MAX;
 std::vector<SessionRenderLightCandidate>lightSnapshots;
 std::vector<SceneMissingAssetDiagnosticCandidate>missingAssets;
};
using SceneGeometryDecorationCandidate=std::function<void(const Graph&,const SessionRenderSceneCandidate&,const std::vector<SceneGeometryModelCandidate>&,const std::function<const SfmModelGeometryCandidate&(const std::string&)>&,PreparedSessionSceneCandidate&)>;
inline PreparedSessionSceneCandidate prepareSessionSceneCandidate(const Graph&graph,const SessionRenderSceneCandidate&scene,const std::function<const SfmModelGeometryCandidate&(const std::string&)>&asset,SceneRenderInputsCacheCandidate&materials,GeometryDeformationWorkerPoolCandidate&workers,const std::vector<std::filesystem::path>&mounts,bool allowScale=false,bool allowNameMapping=false,const SceneModelDeformationCandidate&deform={},const SceneBorrowedDeformationCandidate&borrowed={},const SceneGeometryDecorationCandidate&decorate={}){
 using Clock=std::chrono::steady_clock;auto start=Clock::now();auto milliseconds=[](auto a,auto b){return std::chrono::duration<double,std::milli>(b-a).count();};
 auto skeleton=[&](const std::string&name)->const std::vector<GeometryBoneCandidate>&{return asset(name).bones;};
 auto poses=readSessionScenePosesCandidate(graph,scene,skeleton,allowScale,allowNameMapping);
 auto posesEnd=Clock::now();
 auto geometry=prepareSceneGeometryCandidate(poses,asset,workers,deform,borrowed);
 auto geometryEnd=Clock::now();
 auto inputs=readSceneMaterialInputsCandidate(graph,scene.models,materials,mounts);
 std::vector<SceneMissingAssetDiagnosticCandidate>missingAssets;
 for(size_t model=0;model<inputs.size()&&model<scene.models.size();++model)for(const auto&material:inputs[model].materials)if(!material.source.missingTextureReferences.empty())missingAssets.push_back({scene.models[model].placement.asset,material.source.vmt,material.source.missingTextureReferences});
 auto draws=prepareSceneMaterialDrawInputsCandidate(inputs,mounts);
 auto materialsEnd=Clock::now();
 auto upload=prepareSceneGpuUploadCandidate(geometry,draws,[&](const auto&vertex){return renderCameraSortDistanceCandidate(scene.viewProjection,vertex.position);});
 std::vector<float>alpha;alpha.reserve(upload.materials.size());
 for(const auto&address:upload.materials)alpha.push_back(draws[address.model].alpha.at(address.material));
 PreparedSessionSceneCandidate result{scene.viewProjection,std::move(inputs),std::move(upload),std::move(alpha),scene.width,scene.height};
 result.missingAssets=std::move(missingAssets);
 result.shot=scene.shot;result.lightSnapshots=scene.lights;
 result.profile={0,milliseconds(start,posesEnd),milliseconds(posesEnd,geometryEnd),milliseconds(geometryEnd,materialsEnd),milliseconds(materialsEnd,Clock::now())};if(decorate){auto decorationStart=Clock::now();decorate(graph,scene,geometry,asset,result);result.profile.decoration=milliseconds(decorationStart,Clock::now());}return result;
}
