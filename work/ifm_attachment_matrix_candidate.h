#pragma once
#include "ifm_bounds_frames_candidate.h"
#include "source_model_lighting_point_candidate.h"
struct IfmAttachmentMatrixCandidate {bool valid=false;RecoveredMatrix3x4 matrix{};};
struct IfmAttachmentDescriptorCandidate {uint32_t flags=0;RecoveredMatrix3x4 local{};};
// Complete1023C3D0 selection/math, on an explicitly resolved stable header and
// attachment mapping. Services retains lazy header acquisition and virtual
// model mapping. SetupBones' boolean is ignored; a missing used matrix throws.
template<class Services>
IfmAttachmentMatrixCandidate ifmAttachmentMatrixCandidate(int32_t index,Services&s){
    if(!s.headerAvailable()||index<1||s.attachmentCount()<index){
        const auto position=s.position();const auto angles=s.angles();
        return {false,ifmBoundsAngleMatrixCandidate(angles,position)};
    }
    const float time=static_cast<float>(s.clockTick())*0.0001f;
    (void)s.setupBones(0,0x200,time);
    const auto descriptor=s.descriptor(uint32_t(index-1));
    const auto bone=s.boneWorld(s.mappedBone(uint32_t(index-1)));
    checkIfmMatrixFiniteCandidate(bone);
    RecoveredMatrix3x4 matrix{};
    if(!(descriptor.flags&0x10000)){
        checkIfmMatrixFiniteCandidate(descriptor.local);matrix=composeRecoveredTransforms(bone,descriptor.local);
    }else{
        const auto p=sourceLightingTransformPointCandidate({descriptor.local[0][3],descriptor.local[1][3],descriptor.local[2][3]},bone);
        for(unsigned r=0;r<3;++r){matrix[r][r]=1;matrix[r][3]=p[r];}
    }
    checkIfmMatrixFiniteCandidate(matrix);return {true,matrix};
}
