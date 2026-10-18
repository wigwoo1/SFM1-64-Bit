#pragma once
#include <array>
#include <cmath>
#include <stdexcept>
// Four explicit rows shared with the Vulkan push-constant block.
struct RenderCameraCandidate {
 std::array<float,16>rows;
 std::array<float,4>clip(const std::array<float,3>&position)const{std::array<float,4>out{};for(size_t r=0;r<4;++r){out[r]=rows[r*4]*position[0]+rows[r*4+1]*position[1]+rows[r*4+2]*position[2]+rows[r*4+3];if(!std::isfinite(out[r]))throw std::runtime_error("nonfinite camera projection");}return out;}
 double depth(const std::array<float,3>&position)const{auto p=clip(position);if(p[3]<=0)throw std::runtime_error("camera depth behind eye");return double(p[2])/p[3];}
};
static_assert(sizeof(RenderCameraCandidate)==64,"camera push ABI");
inline RenderCameraCandidate diagnosticRenderCameraCandidate(){return {{.02f,0,0,0,0,0,-.02f,.8f,0,.01f,0,.2f,0,0,0,1}};}
