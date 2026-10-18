#pragma once
#include "session_camera_projection_candidate.h"
#include "camera_rigid_inverse_candidate.h"
#include "graph_local_transform.h"
inline RenderCameraCandidate composeRenderCameraCandidate(const RenderCameraCandidate&a,const RenderCameraCandidate&b){RenderCameraCandidate out{};for(size_t r=0;r<4;++r)for(size_t c=0;c<4;++c){float value=a.rows[r*4]*b.rows[c];for(size_t k=1;k<4;++k)value+=a.rows[r*4+k]*b.rows[k*4+c];if(!std::isfinite(value))throw std::runtime_error("camera composition overflow");out.rows[r*4+c]=value;}return out;}
// Native engine option postrotates Z(-90), then X(+90). Exact axis mapping
// below omits the original trigonometric routine's residual rounding.
inline RecoveredMatrix3x4 engineCameraBasisCandidate(const RecoveredMatrix3x4&input){auto out=input;for(size_t r=0;r<3;++r){out[r][0]=-input[r][1];out[r][1]=input[r][2];out[r][2]=-input[r][0];}return out;}
inline RenderCameraCandidate sessionCameraViewProjectionCandidate(const Graph&graph,uint32_t id,int width,int height,bool engineAxes=false){auto settings=readSessionCameraSettingsCandidate(graph,id);auto&transform=graph.elements.at(settings.transform);if(auto scale=namedAttribute(graph,transform,"scale")){if(scale->type!=3||scale->values.size()!=1||wireFloat(scale->values[0])!=1)throw std::runtime_error("scaled camera view pending");}auto local=graphLocalTransformMatrix(graph,settings.transform);if(engineAxes)local=engineCameraBasisCandidate(local);auto view=cameraRigidInverseCandidate(local);return composeRenderCameraCandidate(sessionCameraViewportProjectionCandidate(graph,id,width,height,true),view);}
