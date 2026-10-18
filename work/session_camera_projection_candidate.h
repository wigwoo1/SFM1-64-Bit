#pragma once
#include "session_camera_settings_candidate.h"
#include "recovered_camera_projection_candidate.h"
#include "viewport_camera_fov_candidate.h"
// Raw CDmeCamera::GetProjectionMatrix convention; callers reproducing the
// engine viewport must use the separately recovered viewport conversion.
inline RenderCameraCandidate sessionCameraProjectionCandidate(const Graph&graph,uint32_t camera,int width,int height,bool vulkanYDown){
 if(width<=0||height<=0)throw std::runtime_error("camera viewport dimensions");auto settings=readSessionCameraSettingsCandidate(graph,camera);if(settings.orthographic)throw std::runtime_error("session orthographic projection pending");return recoveredCameraProjectionCandidate(settings.nearPlane,settings.farPlane,settings.fieldOfView,float(width)/float(height),vulkanYDown);
}
inline RenderCameraCandidate sessionCameraViewportProjectionCandidate(const Graph&graph,uint32_t camera,int width,int height,bool vulkanYDown){
 if(width<=0||height<=0)throw std::runtime_error("camera viewport dimensions");auto settings=readSessionCameraSettingsCandidate(graph,camera);if(settings.orthographic)throw std::runtime_error("session orthographic projection pending");const float aspect=float(width)/float(height);return recoveredCameraProjectionCandidate(settings.nearPlane,settings.farPlane,viewportCameraHorizontalFovCandidate(settings.fieldOfView,aspect),aspect,vulkanYDown);
}
