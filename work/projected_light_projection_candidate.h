#pragma once
#include "projected_light_view_candidate.h"
// SDK MatrixBuildPerspective maps +Z view space directly into a unit cube.
// Algebraically combined negation, add-W and half-scale; native FP parity pending.
inline RenderCameraCandidate projectedLightPerspectiveCandidate(float fovX,float fovY,float nearZ,float farZ){
 for(float v:{fovX,fovY,nearZ,farZ})if(!std::isfinite(v))throw std::runtime_error("nonfinite light projection");
 if(fovX<=0||fovX>=180||fovY<=0||fovY>=180||nearZ<=0||farZ<=nearZ)throw std::runtime_error("light projection range");
 auto tangent=[](float fov){return float(std::tan(double(fov)*3.141592653589793/360.0));};
 float width=2*nearZ*tangent(fovX),height=2*nearZ*tangent(fovY),denominator=nearZ-farZ;
 RenderCameraCandidate p{{-.5f*(2*nearZ/width),0,.5f,0,0,-.5f*(2*nearZ/height),.5f,0,0,0,-farZ/denominator,nearZ*farZ/denominator,0,0,1,0}};
 for(float v:p.rows)if(!std::isfinite(v))throw std::runtime_error("light projection overflow");return p;
}
inline RenderCameraCandidate projectedLightWorldToTextureCandidate(const RecoveredMatrix3x4&world,float fovX,float fovY,float nearZ,float farZ){
 auto p=projectedLightPerspectiveCandidate(fovX,fovY,nearZ,farZ),v=projectedLightViewCandidate(world);RenderCameraCandidate out{};
 for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c){for(unsigned k=0;k<4;++k)out.rows[r*4+c]+=p.rows[r*4+k]*v.rows[k*4+c];if(!std::isfinite(out.rows[r*4+c]))throw std::runtime_error("light matrix overflow");}return out;
}
