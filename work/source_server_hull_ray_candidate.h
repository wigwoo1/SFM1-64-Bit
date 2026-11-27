#pragma once
#include <array>

// Server Ray_t preparation for finite ordered axis-aligned hulls. This is
// geometry preparation only, not collision or entity filtering. Keep the
// original float operation order; other constructors use different arithmetic.
struct SourceServerHullRayCandidate {
    std::array<float,3> center{},delta{},offset{},extents{};
    bool isRay=false,swept=false;
};
inline SourceServerHullRayCandidate sourceServerHullRayCandidate(
    const std::array<float,3>& start,const std::array<float,3>& end,
    const std::array<float,3>& minimum,const std::array<float,3>& maximum) {
    SourceServerHullRayCandidate ray;
    for(unsigned i=0;i<3;++i) {
        ray.delta[i]=end[i]-start[i];
        ray.extents[i]=(maximum[i]-minimum[i])*0.5f;
        const float middle=(minimum[i]+maximum[i])*0.5f;
        ray.center[i]=i==0?start[i]+middle:middle+start[i];
        ray.offset[i]=-1.f*middle;
    }
    const float dx=ray.delta[0]*ray.delta[0],dy=ray.delta[1]*ray.delta[1],dz=ray.delta[2]*ray.delta[2];
    const float dxy=dx+dy; ray.swept=(dxy+dz)!=0.f;
    const float ex=ray.extents[0]*ray.extents[0],ey=ray.extents[1]*ray.extents[1],ez=ray.extents[2]*ray.extents[2];
    const float exy=ex+ey; const float sizeSquared=exy+ez;
    ray.isRay=static_cast<double>(sizeSquared)<1e-6;
    return ray;
}
