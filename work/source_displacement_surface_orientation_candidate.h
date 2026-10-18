#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>

// Owned value data, with no original pointers. Auxiliary field names describe
// dimensions only; their complete material/lightmap meaning is not established.
struct SourceDisplacementSurfaceOrientationCandidate {
 std::array<std::array<float,3>,4>corners{},secondaryVectors{};
 std::array<std::array<float,2>,4>pairs{};
 std::array<float,4>unrotatedScalars{};
 std::array<std::array<float,4>,4>attributes0{},attributes1{};
 std::array<std::array<std::array<float,3>,4>,4>vectors{};
 int32_t startCorner=-1;
 std::array<float,3>start{};
};

// Original 1013CFA0: cached index bypasses nearest search, strict less-than
// preserves the first tie. The native squared-distance sentinel is 1e9f.
inline int32_t sourceDisplacementStartCornerCandidate(const SourceDisplacementSurfaceOrientationCandidate&s){
 if(s.startCorner< -1||s.startCorner>3)throw std::runtime_error("invalid cached displacement start corner");
 if(s.startCorner!=-1)return s.startCorner;
 for(float v:s.start)if(!std::isfinite(v))throw std::runtime_error("nonfinite displacement start");
 float best=1000000000.f;int32_t selected=-1;
 for(int32_t i=0;i<4;++i){std::array<float,3>d{};for(size_t a=0;a<3;++a){if(!std::isfinite(s.corners[i][a]))throw std::runtime_error("nonfinite displacement corner");d[a]=s.start[a]-s.corners[i][a];}
  const float x=d[0]*d[0],y=d[1]*d[1],z=d[2]*d[2],xy=x+y,distance=xy+z;
  if(!std::isfinite(distance))throw std::runtime_error("displacement corner distance overflow");
  if(best>distance){best=distance;selected=i;}
 }return selected;
}

// Original 1013D0C0 rotates all six per-corner groups by the cached index.
// The scalar group at original surface offset108h deliberately stays put.
inline void sourceDisplacementRotateSurfaceCandidate(SourceDisplacementSurfaceOrientationCandidate&s){
 if(s.startCorner<0||s.startCorner>3)throw std::runtime_error("displacement start corner not selected");
 auto result=s;auto rotate=[&](auto&dst,const auto&src){for(size_t i=0;i<4;++i)dst[i]=src[(i+size_t(s.startCorner))%4];};
 rotate(result.corners,s.corners);rotate(result.secondaryVectors,s.secondaryVectors);rotate(result.pairs,s.pairs);rotate(result.attributes0,s.attributes0);rotate(result.attributes1,s.attributes1);rotate(result.vectors,s.vectors);s=result;
}
