#pragma once
#include <cstdint>
#include <cmath>
#include <stdexcept>
// Explicit ramp inputs avoid guessing the lifecycle of native owner+A8/AC.
inline float flexVertexWeightCandidate(float primaryCurrent,float primaryPrevious,float pairCurrent,float pairPrevious,uint8_t speed,uint8_t side){
 for(float v:{primaryCurrent,primaryPrevious,pairCurrent,pairPrevious})if(!std::isfinite(v))throw std::runtime_error("nonfinite vertex flex weight");
 const float unit=1.0f/255.0f,s=float(speed)*unit,t=float(side)*unit;
 const float primary=s*(primaryCurrent-primaryPrevious)+primaryPrevious;
 const float paired=s*(pairCurrent-pairPrevious)+pairPrevious;
 return (paired-primary)*t+primary;
}
