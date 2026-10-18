#pragma once
#include "session_render_light_projection_candidate.h"
#include "projected_light_texture_inputs_candidate.h"
#include "projected_light_prepared_fields_candidate.h"
#include <cstddef>
// Owned, explicitly row-major GPU upload candidate; not an original Source ABI.
struct alignas(16) SceneProjectedLightConstantsCandidate {
 std::array<float,16>worldToTexture{};
 std::array<float,4>originNear{};
 std::array<float,4>colorFar{};
 std::array<float,4>attenuationAmbient{}; // constant, linear, quadratic, ambient
};
static_assert(sizeof(SceneProjectedLightConstantsCandidate)==112);
static_assert(offsetof(SceneProjectedLightConstantsCandidate,originNear)==64);
static_assert(offsetof(SceneProjectedLightConstantsCandidate,colorFar)==80);
static_assert(offsetof(SceneProjectedLightConstantsCandidate,attenuationAmbient)==96);
struct SceneProjectedLightInputCandidate {
 SessionRenderLightCandidate source; // All controls and effects survive preparation.
 SceneProjectedLightConstantsCandidate constants;
 ProjectedLightTextureInputCandidate texture;
 bool requiresShadows=false,requiresUberlight=false,requiresVolumetric=false,requiresPositionJitter=false;
};
inline std::vector<SceneProjectedLightInputCandidate> prepareSceneProjectedLightInputsCandidate(
 const std::vector<SessionRenderLightCandidate>&lights,ProjectedLightTextureInputsCandidate&textures,
 const std::vector<std::filesystem::path>&mounts,uint32_t mip=0){
 std::vector<SceneProjectedLightInputCandidate>result;result.reserve(lights.size());std::set<uint32_t>ids;
 auto scalar=[](uint32_t word){float value;std::memcpy(&value,&word,4);if(!std::isfinite(value))throw std::runtime_error("nonfinite projected light constant");return value;};
 for(const auto&light:lights){
  if(!ids.insert(light.id).second)throw std::runtime_error("duplicate scene light");
  SceneProjectedLightInputCandidate input;input.source=light;
  input.constants.worldToTexture=sessionRenderLightProjectionCandidate(light).rows;
  auto prepared=projectedLightPreparedFieldsCandidate(light.fields,{});
  for(unsigned k=0;k<3;++k){input.constants.originNear[k]=light.world[k][3];input.constants.colorFar[k]=scalar(prepared.rgbWords[k]);}
  input.constants.originNear[3]=scalar(light.fields.floatWords[0]);input.constants.colorFar[3]=scalar(light.fields.floatWords[30]);
  for(unsigned k=0;k<3;++k)input.constants.attenuationAmbient[k]=scalar(light.fields.floatWords[27+k]);
  input.constants.attenuationAmbient[3]=scalar(light.fields.floatWords[3]);
  input.requiresShadows=light.fields.booleanBytes[0]!=0;input.requiresUberlight=light.fields.booleanBytes[2]!=0;input.requiresVolumetric=light.fields.booleanBytes[3]!=0;
  input.requiresPositionJitter=scalar(light.fields.positionJitterWords[0])!=0||scalar(light.fields.positionJitterWords[1])!=0;
  input.texture=textures.prepare(light,mounts,mip);result.push_back(std::move(input));
 }
 return result;
}
