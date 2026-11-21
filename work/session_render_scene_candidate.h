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
#include "session_particle_bool_play_candidate.h"
struct SessionParticleControlCandidate {
 uint32_t id;
 bool simulating,emitting;
};
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
 std::optional<std::array<float,3>> sampledCameraOrigin;
 std::vector<SessionParticleControlCandidate> gameParticles;
};
// Reads every bound model from an already evaluated graph. Asset loading,
// per-model pose evaluation and GPU resource management are separate steps.
inline SessionRenderSceneCandidate readSessionRenderSceneCandidate(const Graph&graph,uint32_t shot,uint32_t width,uint32_t height,
 const SessionCameraSamplePointsCandidate* cameraSamples=nullptr){
 auto binding=readFilmSceneRenderBindingCandidate(graph,shot);
 if(!width||!height)throw std::runtime_error("scene dimensions must be positive");
 SessionRenderSceneCandidate scene{shot,binding.camera,{}, {},width,height};
 if(cameraSamples){
  const auto camera=sessionSampledCameraCandidate(graph,binding.camera,width,height,*cameraSamples);
  scene.viewProjection=camera.viewProjection;
  scene.sampledCameraOrigin=camera.origin;
 }else{
  scene.viewProjection=sessionCameraViewProjectionCandidate(graph,binding.camera,width,height,true);
 }
 scene.lightIds=binding.lights;
 GraphTransformHierarchy hierarchy(graph,binding.scene);scene.models.reserve(binding.models.size());
 for(auto id:binding.models){
  SessionRenderModelCandidate model{id,sessionModelSkinCandidate(graph,id),sessionModelBodyCandidate(graph,id),sessionModelVisibilityCandidate(graph,id),readSessionModelPlacementCandidate(graph,id,hierarchy),sessionModelEvaluateProceduralCandidate(graph,id)};
  model.proxyContext=readSessionModelProxyContextCandidate(graph,id);
  scene.models.push_back(std::move(model));
 }
 for(auto id:binding.lights)scene.lights.push_back(readSessionRenderLightCandidate(graph,id,hierarchy));
 for(auto id:binding.gameParticles){
  const auto& particle=graph.elements.at(id);
  scene.gameParticles.push_back({id,sessionParticleBoolValueCandidate(graph,particle,"simulating"),
                                   sessionParticleBoolValueCandidate(graph,particle,"emitting")});
 }
 return scene;
}
