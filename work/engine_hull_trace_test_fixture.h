#pragma once
#include "engine_point_trace_test_fixture.h"
#include "source_server_hull_ray_candidate.h"
struct EngineHullInputFixture {
 std::array<uint8_t,80> rawRay{};
 SourceServerHullRayCandidate ray;
 uint32_t mask=0,mode=0,excluded=0;
 SourcePhysicsSearchTraceCandidate world;
 std::vector<EnginePointCandidateFixture> candidates;
};
inline bool engineHullReadInput(std::istream& in,EngineHullInputFixture& f){
 if(in.peek()==std::char_traits<char>::eof())return false;
 f={};supportGet(in,f.rawRay);supportGet(in,f.mask);supportGet(in,f.mode);supportGet(in,f.excluded);
 searchReadTrace(in,f.world);uint32_t count;supportGet(in,count);
 if(f.mode>3||count>2048||f.excluded>count||f.rawRay[68]>1||f.rawRay[69]>1)throw std::runtime_error("invalid engine hull input");
 uint32_t transform;std::memcpy(&transform,f.rawRay.data()+64,4);if(transform)throw std::runtime_error("axis transform not supported by fixture");
 std::memcpy(f.ray.center.data(),f.rawRay.data(),12);std::memcpy(f.ray.delta.data(),f.rawRay.data()+16,12);
 std::memcpy(f.ray.offset.data(),f.rawRay.data()+32,12);std::memcpy(f.ray.extents.data(),f.rawRay.data()+48,12);
 f.ray.isRay=f.rawRay[68]!=0;f.ray.swept=f.rawRay[69]!=0;
 for(const auto&v:{f.ray.center,f.ray.delta,f.ray.offset,f.ray.extents})for(float x:v)if(!std::isfinite(x))throw std::runtime_error("nonfinite hull fixture");
 f.candidates.resize(count);for(auto&c:f.candidates){supportGet(in,c.handle);searchReadTrace(in,c.trace);}return true;
}
