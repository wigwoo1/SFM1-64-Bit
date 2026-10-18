#pragma once
#include "render_camera_candidate.h"
// Numeric overload at valve_pyd 0x101BD050: -Z forward, horizontal FOV,
// depth 0..1. std::tan replaces its internal double-precision math routine;
// bitwise native equality has not been established.
inline RenderCameraCandidate recoveredCameraProjectionCandidate(float nearPlane,float farPlane,float fovX,float aspect,bool vulkanYDown){
 if(!std::isfinite(nearPlane)||!std::isfinite(farPlane)||!std::isfinite(fovX)||!std::isfinite(aspect)||nearPlane<=0||farPlane<=nearPlane||fovX<=0||fovX>=180||aspect<=0)throw std::runtime_error("projection parameters");
 double angle=double(fovX)*3.141592653589793*0.002777777777777778;float tangent=float(std::tan(angle));float x=1.0f/tangent;float y=1.0f/(tangent/aspect);float denominator=nearPlane-farPlane;float z=farPlane/denominator;float offset=(nearPlane*farPlane)/denominator;
 RenderCameraCandidate out{{x,0,0,0,0,vulkanYDown?-y:y,0,0,0,0,z,offset,0,0,-1,0}};for(float v:out.rows)if(!std::isfinite(v))throw std::runtime_error("projection overflow");return out;
}
