#pragma once
#include "source_world_light_overflow_candidate.h"
#include "engine_projection_normalize_candidate.h"

struct SourceWorldLightQueryCandidate {
    SourceWorldLightAngularCandidate angular;
    std::array<float,3>position{};
    uint32_t style=0;
    float radius=0,constant=1,linear=0,quadratic=0;
};
inline float sourceWorldLightSquaredCandidate(const std::array<float,3>&v){
    const float x=v[0]*v[0],y=v[1]*v[1],z=v[2]*v[2];const float xy=y+x;return xy+z;
}
// Full100D1240 finite-result domain. x87 denominator intermediates use double
// precision, as in the tested original host environment; do not round each
// multiply/add to float. Surface x-square is rounded before the x87 sum.
inline float sourceWorldLightDistanceCandidate(const SourceWorldLightQueryCandidate&light,
    const std::array<float,3>&delta,bool ignoreRadius){
    for(float x:delta)if(!std::isfinite(x))throw std::runtime_error("nonfinite world-light distance vector");
    for(float x:{light.radius,light.constant,light.linear,light.quadratic})if(!std::isfinite(x))throw std::runtime_error("nonfinite world-light attenuation parameter");
    const int32_t type=light.angular.type;
    if(type!=0&&type!=1&&type!=2&&type!=4)return 1;
    const float squared=sourceWorldLightSquaredCandidate(delta);
    if(!std::isfinite(squared))throw std::runtime_error("world-light squared distance overflow");
    if(type==0){
        const float radiusSquared=light.radius*light.radius;
        if(light.radius!=0&&squared>radiusSquared)return 0;
        if(squared<1)return 1;
        const float xSquared=delta[0]*delta[0];
        const double yy=double(delta[1])*delta[1],zz=double(delta[2])*delta[2];
        return float(1.0/((yy+double(xSquared))+zz));
    }
    const float distance=std::sqrt(squared);
    if(type==4){const float value=light.linear-distance;return value<0?0:value;}
    if(!ignoreRadius&&light.radius!=0&&distance>light.radius)return 0;
    const double linear=double(light.linear)*distance;
    const double quadratic=double(light.quadratic)*squared;
    const double denominator=(linear+double(light.constant))+quadratic;
    if(denominator==0)throw std::runtime_error("zero world-light attenuation denominator");
    const float value=float(1.0/denominator);
    if(!std::isfinite(value))throw std::runtime_error("world-light attenuation overflow");return value;
}
enum class SourceWorldLightTraceModeCandidate:uint32_t { WorldOnly=1,EverythingFilterProps=3 };
struct SourceWorldLightRayCandidate {
    std::array<float,3>start{},delta{};
    bool swept=false;
};
struct SourceWorldLightFilterCandidate {
    SourceWorldLightTraceModeCandidate mode=SourceWorldLightTraceModeCandidate::WorldOnly;
    uintptr_t excludedEntity=0;
};
struct SourceWorldLightTraceCandidate {
    float fraction=1;
    uint16_t surfaceFlags=0;
    std::array<float,3>endPosition{};
};
// Full10194420 policy with renderer/world dependencies supplied explicitly.
// Trace service must implement the requested filter, including static-prop
// filtering for mode3. A world-only collision adapter cannot silently replace it.
template<class Services>
SourceWorldLightVisibilityCandidate sourceWorldLightVisibilityQueryCandidate(
    const SourceWorldLightQueryCandidate&light,const std::array<float,3>&point,uint32_t flags,
    uintptr_t excludedEntity,Services&services,std::array<float,3>initialIncoming={}){
    if(flags&~15u)throw std::runtime_error("unknown world-light query flags");
    for(float x:point)if(!std::isfinite(x))throw std::runtime_error("nonfinite world-light query point");
    SourceWorldLightVisibilityCandidate out{initialIncoming,0};
    if(light.angular.type==5)return out; // Does not initialize the incoming vector.
    SourceWorldLightFilterCandidate filter;
    if(flags&4){filter.mode=SourceWorldLightTraceModeCandidate::EverythingFilterProps;filter.excludedEntity=excludedEntity;}
    auto trace=[&](const std::array<float,3>&delta){
        for(float x:delta)if(!std::isfinite(x))throw std::runtime_error("nonfinite world-light ray");
        const float squared=sourceWorldLightSquaredCandidate(delta);
        const auto result=services.trace(SourceWorldLightRayCandidate{point,delta,squared!=0},0x4481u,filter);
        if(!std::isfinite(result.fraction)||result.fraction<0||result.fraction>1)throw std::runtime_error("invalid world-light trace fraction");
        return result;
    };
    if(light.angular.type==3){
        out.incoming={0,0,0};std::array<float,3>delta{};
        for(size_t i=0;i<3;++i){const float step=light.angular.normal[i]*57016.3203125f;const float end=point[i]-step;delta[i]=end-point[i];}
        const auto hit=trace(delta);
        if(hit.surfaceFlags&4){for(size_t i=0;i<3;++i)out.incoming[i]=-light.angular.normal[i];out.attenuation=1;}
        return out;
    }
    for(size_t i=0;i<3;++i)out.incoming[i]=light.position[i]-point[i];
    const auto rawDelta=out.incoming;
    float attenuation=sourceWorldLightDistanceCandidate(light,rawDelta,(flags&2)!=0);
    if(!(flags&8)){
        const float style=float(services.style(light.style));const float scaled=style*0.0037878789007663727f;
        attenuation=scaled*attenuation;
    }
    const float maximum=std::max(std::max(light.angular.intensity[0],light.angular.intensity[1]),light.angular.intensity[2]);
    if(light.angular.type!=0){const float brightness=maximum*attenuation;if(services.intensityThreshold()>brightness)return out;}
    const float distance=engineProjectionNormalizeCandidate(out.incoming.data());
    if(flags&1){out.attenuation=attenuation;return out;}
    const auto hit=trace(rawDelta);const float remaining=1.f-hit.fraction;const float obstruction=remaining*distance;
    if(obstruction<=8){out.attenuation=attenuation;return out;}
    if(services.debugMode()==2)services.debugLine(point,hit.endPosition);
    return out;
}

// Composition boundary matching full10195510 ->10194420 ->100D1420/10193390.
// The original overflow caller has no excluded entity (ECX is zero).
template<class Services>
void sourceWorldLightCompleteOverflowCandidate(const SourceWorldLightQueryCandidate&light,
    const std::array<float,3>&point,SourceModelLightingStateCandidate&state,bool special,Services&services){
    struct Binding {
        const SourceWorldLightQueryCandidate&light;Services&services;
        SourceWorldLightVisibilityCandidate visibility(const SourceWorldLightAngularCandidate&,const std::array<float,3>&p,uint32_t flags){return sourceWorldLightVisibilityQueryCandidate(light,p,flags,0,services);}
        decltype(auto) directions(){return services.directions();}
        int32_t directionCount(){return services.directionCount();}
    }binding{light,services};
    sourceWorldLightOverflowCandidate(light.angular,point,state,special,binding);
}
