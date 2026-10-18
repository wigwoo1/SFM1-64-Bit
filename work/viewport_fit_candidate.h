#pragma once
#include <cstdint>
#include <climits>
#include <stdexcept>
struct ViewportFitCandidate {int32_t x,y,width,height;};
inline ViewportFitCandidate viewportFitCandidate(uint32_t sw,uint32_t sh,uint32_t dw,uint32_t dh){
 if(!sw||!sh||!dw||!dh||dw>INT_MAX||dh>INT_MAX)throw std::runtime_error("invalid viewport extent");
 uint64_t w=dw,h=dh;
 if(uint64_t(dw)*sh>uint64_t(dh)*sw)w=uint64_t(dh)*sw/sh;else h=uint64_t(dw)*sh/sw;
 if(!w||!h)throw std::runtime_error("viewport aspect ratio cannot fit one pixel");
 return {int32_t((dw-w)/2),int32_t((dh-h)/2),int32_t(w),int32_t(h)};
}
