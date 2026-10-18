#pragma once
#include "camera_rigid_inverse_candidate.h"
// Valve SDK clientshadowmgr.cpp BuildWorldToShadowMatrix basis permutation:
// columns (forward,left,up) become (left,up,forward), then rigid inverse.
// Installed SFM equality and perspective/gobo transforms remain unverified.
inline RenderCameraCandidate projectedLightViewCandidate(const RecoveredMatrix3x4&world){
 auto permuted=world;
 for(unsigned r=0;r<3;++r){permuted[r][0]=world[r][1];permuted[r][1]=world[r][2];permuted[r][2]=world[r][0];}
 return cameraRigidInverseCandidate(permuted);
}
