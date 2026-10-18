#pragma once
// Test-only reproducible semantic worldlights and direction/visibility replies.
#include "source_world_light_overflow_candidate.h"
inline SourceWorldLightAngularCandidate overflowFixtureLight(uint32_t id){
    SourceWorldLightAngularCandidate l;l.type=int32_t((id-1)%6);l.intensity={float(id),float(id)*.5f,float(id)*.25f};l.normal={0,0,-.5f};l.inner=.75f;l.outer=.25f;l.exponent=(id%2)?.5f:2.f;return l;
}
inline SourceWorldLightVisibilityCandidate overflowFixtureVisibility(uint32_t id,const std::array<float,3>&point){
    const float base=float(id)*.125f,extra=std::abs(point[0])*.0009765625f;
    return {{{float(int32_t(id%3)-1)*.25f,float(id%2)*.5f,1.f}},base+extra};
}
struct OverflowFixtureRenderServices {
    uint32_t id;const std::array<float,3>&point;
    std::array<std::array<float,3>,6>basis{{{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}}};
    SourceWorldLightVisibilityCandidate visibility(const SourceWorldLightAngularCandidate&,const std::array<float,3>&p,uint32_t flags){if(p!=point||flags)throw std::runtime_error("composite visibility arguments");return overflowFixtureVisibility(id,p);}
    const auto&directions(){return basis;}
    int32_t directionCount(){return 6;}
};
