#pragma once
#include "session_render_light_candidate.h"
#include "projected_light_projection_candidate.h"
// Installed conversion: minDistance cached +D0 -> output +1C;
// inherited maxDistance +C8 -> +20; FOV max is copied to +24/+28.
// Near/far semantic mapping follows SDK FlashlightState layout, not a full
// installed renderer execution oracle. World pose is unjittered here.
inline RenderCameraCandidate sessionRenderLightProjectionCandidate(const SessionRenderLightCandidate&light){
 auto scalar=[](uint32_t word){float value;std::memcpy(&value,&word,4);return value;};
 float nearZ=scalar(light.fields.floatWords[0]),farZ=scalar(light.fields.floatWords[30]);
 float horizontal=scalar(light.fields.floatWords[1]),vertical=scalar(light.fields.floatWords[2]);
 float fov=horizontal>vertical?horizontal:vertical;
 return projectedLightWorldToTextureCandidate(light.world,fov,fov,nearZ,farZ);
}
