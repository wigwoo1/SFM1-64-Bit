#pragma once
#include "source_model_cached_lighting_candidate.h"
#include <algorithm>

// Only fields consumed by100D1420/10193390 are represented here. Position,
// distance attenuation, styles and ray visibility belong to the query service.
struct SourceWorldLightAngularCandidate {
    int32_t type=1;
    std::array<float,3> intensity{},normal{};
    float inner=1,outer=0,exponent=1;
};
inline float sourceWorldLightDotCandidate(const std::array<float,3>&a,const std::array<float,3>&b){
    const float x=a[0]*b[0],y=a[1]*b[1],z=a[2]*b[2];const float xy=y+x;return xy+z;
}
// Original100D1420. The overflow caller supplies the same incoming vector as
// both receiving normal and direction, NOT a model vertex normal.
inline float sourceWorldLightAngularCandidate(const SourceWorldLightAngularCandidate&light,
    const std::array<float,3>&emissionNormal,const std::array<float,3>&receivingNormal,
    const std::array<float,3>&incoming){
    for(const auto*p:{&emissionNormal,&receivingNormal,&incoming})for(float x:*p)if(!std::isfinite(x))throw std::runtime_error("nonfinite light direction");
    if(!std::isfinite(light.inner)||!std::isfinite(light.outer)||!std::isfinite(light.exponent))throw std::runtime_error("nonfinite light cone");
    if(light.type<0||light.type>5)return 0;
    if(light.type==5)return 1;
    const float dot=sourceWorldLightDotCandidate(receivingNormal,incoming);
    if(light.type==3){const float cosine=-sourceWorldLightDotCandidate(emissionNormal,receivingNormal);return cosine<0?0:cosine;}
    if(dot<0)return 0;
    if(light.type==0){
        // Original x87 y*x + x*x + z*x at the caller's 53-bit precision.
        const double y=double(emissionNormal[1])*incoming[1],x=double(emissionNormal[0])*incoming[0],z=double(emissionNormal[2])*incoming[2];
        const double cosine=-((y+x)+z);return cosine>0.01?float(cosine*dot):0;
    }
    if(light.type==1||light.type==4)return dot;
    const float cosine=-sourceWorldLightDotCandidate(emissionNormal,incoming);
    if(light.outer>=cosine)return 0;
    if(cosine>=light.inner)return dot;
    if(light.exponent==0||light.exponent==1){const float numerator=cosine-light.outer,denominator=light.inner-light.outer;const float ramp=numerator/denominator;return ramp*dot;}
    const double ramp=(double(cosine)-double(light.outer))/(double(light.inner)-double(light.outer));
    return float(std::pow(ramp,double(light.exponent))*double(dot));
}
struct SourceWorldLightVisibilityCandidate {
    std::array<float,3>incoming{};
    float attenuation=0;
};
// Original10193390, a maximum six-direction destination. Exact-zero scale
// returns before either basis virtual query. Basis order is supplied by the
// renderer; it is not assumed to be a particular hardcoded cube ordering.
template<class Services>
void sourceWorldLightAmbientAddCandidate(const SourceWorldLightAngularCandidate&light,
    const std::array<float,3>&incoming,float scale,std::array<float,18>&ambient,Services&services){
    if(scale==0)return;
    if(!std::isfinite(scale))throw std::runtime_error("nonfinite overflow attenuation");
    const auto&directions=services.directions();const int32_t count=services.directionCount();
    if(count<0||count>6||size_t(count)>directions.size())throw std::runtime_error("ambient basis extent");
    for(int32_t i=count-1;i>=0;--i){
        const float dot=sourceWorldLightDotCandidate(directions[size_t(i)],incoming);
        if(dot>0){const float amount=dot*scale;for(unsigned channel=0;channel<3;++channel){const float term=light.intensity[channel]*amount;const float combined=term+ambient[size_t(i)*3+channel];if(!std::isfinite(combined))throw std::runtime_error("overflow ambient addition overflow");ambient[size_t(i)*3+channel]=combined;}}
    }
}
template<class Services>
void sourceWorldLightOverflowCandidate(const SourceWorldLightAngularCandidate&light,
    const std::array<float,3>&point,SourceModelLightingStateCandidate&state,bool special,Services&services){
    const auto query=services.visibility(light,point,special?3u:0u);
    if(!std::isfinite(query.attenuation))throw std::runtime_error("nonfinite visibility attenuation");
    for(float x:light.intensity)if(!std::isfinite(x))throw std::runtime_error("nonfinite light intensity");
    const float angular=sourceWorldLightAngularCandidate(light,light.normal,query.incoming,query.incoming);
    const float scale=angular*query.attenuation;
    sourceWorldLightAmbientAddCandidate(light,query.incoming,scale,state.ambient,services);
}
