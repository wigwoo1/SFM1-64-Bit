#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
// Stored sign bits choose the support corners; do not silently recompute them.
inline uint32_t sourceBoundsPlaneSideCandidate(const std::array<float,3>&mins,const std::array<float,3>&maxs,
 const std::array<float,3>&normal,float distance,uint8_t type,uint8_t signs){
 if(type>5||signs>7||!std::isfinite(distance))throw std::runtime_error("invalid bounds plane metadata");for(size_t i=0;i<3;++i)if(!std::isfinite(mins[i])||!std::isfinite(maxs[i])||!std::isfinite(normal[i])||mins[i]>maxs[i])throw std::runtime_error("invalid plane bounds");
 if(type<3){if(mins[type]>=distance)return 1;if(distance>=maxs[type])return 2;return 3;}
 auto dot=[&](bool high){std::array<float,3>p{};for(size_t i=0;i<3;++i){bool negative=(signs&(1u<<i))!=0;p[i]=(high!=negative?maxs[i]:mins[i])*normal[i];}const float xy=p[1]+p[0];const float value=xy+p[2];if(!std::isfinite(value))throw std::runtime_error("bounds dot overflow");return value;};
 return (dot(true)>=distance?1u:0u)|(dot(false)<distance?2u:0u);
}
