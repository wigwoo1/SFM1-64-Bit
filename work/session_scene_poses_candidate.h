#pragma once
#include "session_render_scene_candidate.h"
#include "session_bone_pose_candidate.h"
#include <functional>
struct SessionScenePoseModelCandidate {SessionRenderModelCandidate model;std::vector<GeometryMatrixCandidate>locals;};
struct SessionScenePosesCandidate {uint32_t shot;RenderCameraCandidate camera;std::vector<SessionScenePoseModelCandidate>models;};
// All-or-error extraction: never silently drop a model whose pose is missing.
// Procedural evaluation remains a subsequent stage, not a rigid fallback.
inline SessionScenePosesCandidate readSessionScenePosesCandidate(const Graph&graph,const SessionRenderSceneCandidate&scene,const std::function<const std::vector<GeometryBoneCandidate>&(const std::string&)>&skeleton,bool allowScale=false,bool allowNameMapping=false){
 if(!skeleton)throw std::runtime_error("scene skeleton loader required");SessionScenePosesCandidate result{scene.shot,scene.viewProjection,{}};result.models.reserve(scene.models.size());
 for(auto&model:scene.models){auto&bones=skeleton(model.placement.asset);auto locals=readSessionBonePoseCandidate(graph,model.id,bones,allowScale,allowNameMapping);result.models.push_back({model,std::move(locals)});}return result;
}
