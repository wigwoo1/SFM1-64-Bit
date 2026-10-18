#pragma once
#include "ik_vector_normalize.h"
struct RecoveredIkBendCandidate{std::array<float,3> offset,plane;float along,height;};
// Native 105D0D2E..105D0E80; caller has already capped distance to reach.
inline RecoveredIkBendCandidate recoveredIkBendCandidate(
 const std::array<float,3>&direction,const std::array<float,3>&pole,
 float upperLength,float lowerLength,float distance){
 const float dot=(direction[0]*pole[0]+pole[1]*direction[1])+pole[2]*direction[2];
 std::array<float,3>plane{};for(unsigned i=0;i<3;++i)plane[i]=pole[i]-direction[i]*dot;
 normalizeRecoveredIkVectorCandidate(plane);
 const float lowerSquare=lowerLength*lowerLength;
 const float distanceSquare=distance*distance;
 const float twiceDistance=distance*2.0f;
 const float upperSquare=upperLength*upperLength;
 const float difference=upperSquare-lowerSquare;
 const float numerator=difference+distanceSquare;
 const float along=numerator/twiceDistance;
 const float alongSquare=along*along;
 float heightSquare=upperSquare-alongSquare;
 if(0.0f>heightSquare)heightSquare=0.0f;
 const float height=std::sqrt(heightSquare);
 std::array<float,3>offset{};for(unsigned i=0;i<3;++i){
  const float lateral=plane[i]*height,forward=direction[i]*along;offset[i]=forward+lateral;}
 return {offset,plane,along,height};
}
