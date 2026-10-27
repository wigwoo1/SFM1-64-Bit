#pragma once
#include "film_scene_render_binding_candidate.h"
#include "session_model_placement_candidate.h"
#include "session_camera_view_projection_candidate.h"
#include "session_model_visibility_candidate.h"
#include "session_model_skin_candidate.h"
#include "session_model_body_candidate.h"
#include "session_model_procedural_candidate.h"
#include "session_render_light_candidate.h"
#include "session_model_proxy_context_candidate.h"
struct SessionRenderModelCandidate {
 uint32_t id,skin,body;bool visible;SessionModelPlacementCandidate placement;bool evaluateProcedural=true;
 SessionModelProxyContextCandidate proxyContext;
};
struct SessionRenderSceneCandidate {
 uint32_t shot,camera;RenderCameraCandidate viewProjection;
 std::vector<SessionRenderModelCandidate>models;
 uint32_t width=512,height=512;
 std::vector<uint32_t>lightIds;
 std::vector<SessionRenderLightCandidate>lights;
};
// Reads every bound model from an already evaluated graph. Asset loading,
// per-model pose evaluation and GPU resource management are separate steps.
inline SessionRenderSceneCandidate readSessionRenderSceneCandidate(const Graph&graph,uint32_t shot,uint32_t width,uint32_t height){
 auto binding=readFilmSceneRenderBindingCandidate(graph,shot);
 if(!width||!height)throw std::runtime_error("scene dimensions must be positive");
 SessionRenderSceneCandidate scene{shot,binding.camera,sessionCameraViewProjectionCandidate(graph,binding.camera,width,height,true),{},width,height};
 scene.lightIds=binding.lights;
 GraphTransformHierarchy hierarchy(graph,binding.scene);scene.models.reserve(binding.models.size());
 for(auto id:binding.models){
  SessionRenderModelCandidate model{id,sessionModelSkinCandidate(graph,id),sessionModelBodyCandidate(graph,id),sessionModelVisibilityCandidate(graph,id),readSessionModelPlacementCandidate(graph,id,hierarchy),sessionModelEvaluateProceduralCandidate(graph,id)};
  model.proxyContext=readSessionModelProxyContextCandidate(graph,id);
  scene.models.push_back(std::move(model));
 }
 for(auto id:binding.lights)scene.lights.push_back(readSessionRenderLightCandidate(graph,id,hierarchy));
 return scene;
}
