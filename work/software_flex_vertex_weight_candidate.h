#pragma once
#include <cstdint>
#include <cmath>
#include <stdexcept>
#include <initializer_list>
// R_StudioFlexVerts scalar SSE operation order. A8/AC temporal identity is
// unresolved; explicit array labels avoid asserting current/previous semantics.
inline float softwareFlexVertexWeightCandidate(float primaryA8,float primaryAC,float pairA8,float pairAC,uint8_t speed,uint8_t side){
 for(float value:{primaryA8,primaryAC,pairA8,pairAC})if(!std::isfinite(value))throw std::runtime_error("nonfinite software flex coefficient");
 const float unit=1.0f/255.0f;float s=float(speed)*unit,t=float(side)*unit;
 float primary=primaryAC*(1.0f-s)+primaryA8*s;
 float paired=pairAC*(1.0f-s)+pairA8*s;
 return primary*(1.0f-t)+paired*t;
}
