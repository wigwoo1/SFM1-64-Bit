#pragma once
#include "source_physics_refinement_face_candidate.h"
#include "source_physics_triangle_contact_time_candidate.h"
#include "source_physics_support_candidate.h"

struct SourcePhysicsBoxSupportCandidate {
    std::array<std::array<float,2>,3>axes{};
    bool point=true;
};
// Original27270: sign BIT chooses a stored bound, including negative zero.
inline SourcePhysicsSupportResultCandidate sourcePhysicsBoxSupportCandidate(
    const SourcePhysicsBoxSupportCandidate&box,const std::array<float,3>&direction){
    SourcePhysicsSupportResultCandidate result;if(box.point)return result;
    for(unsigned axis=0;axis<3;++axis){if(!std::isfinite(direction[axis]))throw std::runtime_error("nonfinite box support direction");const unsigned side=std::signbit(direction[axis])?1u:0u;result.point[axis]=box.axes[axis][side];if(!std::isfinite(result.point[axis]))throw std::runtime_error("nonfinite box support bound");result.feature|=side<<axis;}return result;
}

// Complete original28F40 refinement. ray is original27310's16 floats.
// Local copies make owned failure transactional; successful/iteration-limit
// returns commit all simplex/cache changes, matching the original side effects.
inline float sourcePhysicsRefinementCandidate(SourcePhysicsSimplexCandidate&simplex,
    const std::array<float,16>&ray,const SourcePhysicsSupportContextCandidate&shape,
    const SourcePhysicsBoxSupportCandidate&box,SourcePhysicsSupportCacheCandidate&cache){
    if(int32_t(simplex.count)<4)return 0.f;
    using V=std::array<float,3>;auto finite=[](float x){if(!std::isfinite(x))throw std::runtime_error("nonfinite physics refinement arithmetic");return x;};
    for(unsigned i=0;i<12;++i)finite(ray[i]);
    auto dot=[&](const V&a,const V&b){return finite((a[1]*b[1]+a[0]*b[0])+a[2]*b[2]);};
    auto subtract=[&](const V&a,const V&b){return V{finite(a[0]-b[0]),finite(a[1]-b[1]),finite(a[2]-b[2])};};
    auto cross=[&](const V&a,const V&b){return V{finite(a[1]*b[2]-a[2]*b[1]),finite(a[2]*b[0]-a[0]*b[2]),finite(a[0]*b[1]-a[1]*b[0])};};
    auto state=simplex;auto nextCache=cache;const V delta{ray[6],ray[7],ray[8]},unit{ray[9],ray[10],ray[11]};
    V normal=sourcePhysicsRefinementFaceCandidate(state,unit,true);
    auto commit=[&](float answer){simplex=state;cache=nextCache;return answer;};
    for(unsigned iteration=0;iteration<100;++iteration){
        normal=sourcePhysicsContactNormalizeCandidate(normal);
        // Original295F0 binds moving box at+64 and fixed compact shape at+68.
        const auto supportA=sourcePhysicsBoxSupportCandidate(box,normal);const bool end=dot(delta,normal)>0;
        const V endpoint{ray[end?3:0],ray[end?4:1],ray[end?5:2]};const V opposite{-normal[0],-normal[1],-normal[2]};const auto supportB=sourcePhysicsSupportCandidate(shape,opposite,nextCache);
        SourcePhysicsSimplexVertexCandidate incoming;for(unsigned axis=0;axis<3;++axis)incoming.point[axis]=finite(finite(supportA.point[axis]+endpoint[axis])-supportB.point[axis]);incoming.featureA=uint16_t((supportA.feature&0x7fff)|(end?0x8000:0));incoming.featureB=uint16_t(supportB.feature);
        const float incomingProjection=dot(incoming.point,normal),faceProjection=finite(dot(state.vertices[0].point,normal)+.00390625f);
        if(faceProjection>=incomingProjection){
            const auto&a=state.vertices[0].point;const V face=cross(subtract(state.vertices[1].point,a),subtract(state.vertices[2].point,a));const float denominator=dot(unit,face);
            return commit(denominator>0?finite(dot(a,face)/denominator):1.0000000138484279e24f);
        }
        if(state.count!=3)throw std::runtime_error("refinement append count");state.vertices[3]=incoming;state.count=4;normal=sourcePhysicsRefinementFaceCandidate(state,unit,false);
    }
    return commit(0.f);
}
