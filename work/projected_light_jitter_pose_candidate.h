#pragma once
#include "quaternion_angles_candidate.h"
#include "angle_basis_candidate.h"
#include "projected_light_jitter_displacement_candidate.h"
// Returns supplied world quaternion and jittered position. Saved snapshot only.
inline std::array<uint32_t,7> projectedLightJitterPoseCandidate(const ProjectedLightSavedFieldsCandidate& snapshot,const std::array<uint32_t,7>& pose){
    if((snapshot.positionJitterWords[0]&0x7fffffffu)==0&&(snapshot.positionJitterWords[1]&0x7fffffffu)==0)return pose;
    const std::array<uint32_t,4> quaternion{{pose[0],pose[1],pose[2],pose[3]}};
    const auto angles=quaternionAnglesCandidate(quaternion);
    const auto basis=angleBasisCandidate(angles);
    const std::array<uint32_t,6> rightUp{{basis[3],basis[4],basis[5],basis[6],basis[7],basis[8]}};
    const std::array<uint32_t,3> origin{{pose[4],pose[5],pose[6]}};
    const auto position=projectedLightJitterDisplacementCandidate(origin,rightUp,snapshot.positionJitterWords,snapshot.floatWords[4]);
    auto result=pose;for(unsigned i=0;i<3;++i)result[4+i]=position[i];return result;
}
