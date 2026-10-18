#pragma once
#include <array>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <stdexcept>
struct ModelEyeballCandidate {uint32_t body,model,bone;std::array<float,3>origin,up,forward;float zoffset,radius,irisScale;int32_t texture;};
inline ModelEyeballCandidate readModelEyeballCandidate(const std::vector<uint8_t>&bytes,size_t offset,uint32_t body,uint32_t model,size_t boneCount){
 if(offset>bytes.size()||bytes.size()-offset<172)throw std::runtime_error("MDL eyeball span");
 auto scalar=[&](size_t field){float value;std::memcpy(&value,bytes.data()+offset+field,4);if(!std::isfinite(value))throw std::runtime_error("nonfinite eyeball metadata");return value;};
 auto integer=[&](size_t field){int32_t value;std::memcpy(&value,bytes.data()+offset+field,4);return value;};
 auto vector=[&](size_t field){return std::array<float,3>{scalar(field),scalar(field+4),scalar(field+8)};};
 auto bone=integer(4);if(bone<0||size_t(bone)>=boneCount)throw std::runtime_error("eyeball bone index");
 ModelEyeballCandidate eye{body,model,uint32_t(bone),vector(8),vector(28),vector(40),scalar(20),scalar(24),scalar(60),integer(52)};
 if(!(eye.radius>0)||!(eye.irisScale>0))throw std::runtime_error("eyeball projection scale");return eye;
}
