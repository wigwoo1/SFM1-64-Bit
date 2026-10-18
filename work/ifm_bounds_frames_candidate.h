#pragma once
#include "ifm_animated_hitbox_bounds_candidate.h"
#include <optional>
#include <cstdint>
#include <cstring>
extern "C" void sfm64OriginalStyleSinCos(const float* radians,float* sine,float* cosine);

// Original104992B0/1049D830: degrees (pitch,yaw,roll), row-major matrix.
// This differs from AngleVectors' right-vector sign. Preserve scalar binary32
// operations and the original hardware FSINCOS via the existing x64 helper.
// The finite angle domain is deliberately bounded pending broader verification.
inline RecoveredMatrix3x4 ifmBoundsAngleMatrixCandidate(
    const std::array<float,3>&angles,const std::array<float,3>&position){
    for(float x:angles)if(!(x>=-36000.0f&&x<=36000.0f))throw std::runtime_error("bounds angle outside verified domain");
    for(float x:position)if(!std::isfinite(x))throw std::runtime_error("nonfinite bounds frame position");
    const uint32_t factorBits=0x3c8efa35;float factor;std::memcpy(&factor,&factorBits,4);
    float sy,cy,sp,cp,sr,cr;
    float radians=angles[1]*factor;sfm64OriginalStyleSinCos(&radians,&sy,&cy);
    radians=angles[0]*factor;sfm64OriginalStyleSinCos(&radians,&sp,&cp);
    radians=angles[2]*factor;sfm64OriginalStyleSinCos(&radians,&sr,&cr);
    RecoveredMatrix3x4 m{};m[0][0]=cp*cy;m[1][0]=cp*sy;
    uint32_t spBits;std::memcpy(&spBits,&sp,4);spBits^=0x80000000u;std::memcpy(&m[2][0],&spBits,4);
    const float srsp=sr*sp;m[0][1]=(srsp*cy)-(cr*sy);m[1][1]=(srsp*sy)+(cr*cy);m[2][1]=sr*cp;
    const float crsp=cr*sp;m[1][2]=(crsp*sy)-(sr*cy);m[0][2]=(crsp*cy)+(sr*sy);m[2][2]=cr*cp;
    for(unsigned i=0;i<3;++i)m[i][3]=position[i];return m;
}
struct IfmBoundsWrapperFramesCandidate {
    // Native renderable base-object offsets, not established live field names.
    std::array<float,3>position38{},angles44{},position50{},angles5c{};
};
// Complete1023B640. Rigid transpose inverse here, without the scale correction
// used by the separate child-override branch below.
inline RecoveredMatrix3x4 ifmBoundsWrapperMatrixCandidate(const IfmBoundsWrapperFramesCandidate&frames){
    const auto a=ifmBoundsAngleMatrixCandidate(frames.angles5c,frames.position50);
    const auto b=ifmBoundsAngleMatrixCandidate(frames.angles44,frames.position38);
    auto result=composeRecoveredTransforms(invertRecoveredTransposeTransform(b),a);
    checkIfmMatrixFiniteCandidate(result);return result;
}
// Original104C3D50. overridePos/overrideRot are intentionally absent: this
// predicate checks disableOverride and the typed overrideParent reference only.
inline bool ifmBoundsChildOverrideCandidate(bool disabled,bool typedOverrideParentResolved){
    return !disabled&&typedOverrideParentResolved;
}
// Original1023BCD0 selection/composition. Services bind resolved DAG identity,
// raw child index0, typed override reference and local/world transform inputs.
// A null/untyped child0 is an explicit invalid-input error, not a later-child
// search (the original dereferences it). No live engine/datamodel ABI is implied.
template<class Services>
std::optional<RecoveredMatrix3x4> ifmBoundsChildMatrixCandidate(int32_t runtimeMode,Services&services){
    const auto dag=services.resolveDag(); // Original resolves even in mode0.
    if(runtimeMode==0||!dag||services.childCount(*dag)<=0)return {};
    const auto child=services.typedChildZero(*dag);
    if(!child)throw std::runtime_error("bounds child0 is not a resolved DAG");
    RecoveredMatrix3x4 result;
    if(services.overrideParentEnabled(*child)){
        const auto childWorld=services.world(*child),parentWorld=services.world(*dag);
        checkIfmMatrixFiniteCandidate(childWorld);checkIfmMatrixFiniteCandidate(parentWorld);
        const auto inverse=recoveredIkCoordinateInverseCandidate(parentWorld);
        checkIfmMatrixFiniteCandidate(inverse);result=composeRecoveredTransforms(inverse,childWorld);
    }else result=services.local(*child);
    checkIfmMatrixFiniteCandidate(result);return result;
}
template<class Services>
IfmBoundsCandidate ifmBoundsAdjustChildCandidate(int32_t runtimeMode,Services&services,const IfmBoundsCandidate&bounds){
    const auto matrix=ifmBoundsChildMatrixCandidate(runtimeMode,services);
    return matrix?ifmTransformBoundsCandidate(*matrix,bounds):bounds;
}
