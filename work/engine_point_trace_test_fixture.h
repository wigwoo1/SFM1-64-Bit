#pragma once
#include "source_engine_point_trace_candidate.h"
#include "physics_search_test_fixture.h"
struct EnginePointCandidateFixture {uint32_t handle=0;SourcePhysicsSearchTraceCandidate trace;};
struct EnginePointInputFixture {
    SourceWorldLightRayCandidate ray;
    uint32_t mask=0,mode=1,excluded=0;
    SourcePhysicsSearchTraceCandidate world;
    std::vector<EnginePointCandidateFixture>candidates;
};
inline void enginePointWriteInput(std::ostream&out,const EnginePointInputFixture&f){supportPut(out,f.ray.start);supportPut(out,f.ray.delta);supportPut(out,uint32_t(f.ray.swept));supportPut(out,f.mask);supportPut(out,f.mode);supportPut(out,f.excluded);searchWriteTrace(out,f.world);supportPut(out,uint32_t(f.candidates.size()));for(const auto&c:f.candidates){supportPut(out,c.handle);searchWriteTrace(out,c.trace);}}
inline bool enginePointReadInput(std::istream&in,EnginePointInputFixture&f){if(in.peek()==std::char_traits<char>::eof())return false;f={};supportGet(in,f.ray.start);supportGet(in,f.ray.delta);uint32_t swept,n;supportGet(in,swept);supportGet(in,f.mask);supportGet(in,f.mode);supportGet(in,f.excluded);searchReadTrace(in,f.world);supportGet(in,n);if(swept>1||(f.mode!=1&&f.mode!=3)||n>2048||f.excluded>n)throw std::runtime_error("engine point fixture flags/size");f.ray.swept=swept!=0;f.candidates.resize(n);for(auto&c:f.candidates){supportGet(in,c.handle);searchReadTrace(in,c.trace);}for(const auto&v:{f.ray.start,f.ray.delta})for(float x:v)if(!std::isfinite(x))throw std::runtime_error("nonfinite engine point query");auto validate=[](const auto&t){for(const auto&v:{t.start,t.end,t.plane.normal})for(float x:v)if(!std::isfinite(x))throw std::runtime_error("nonfinite test trace vector");for(float x:{t.plane.distance,t.fraction,t.fractionLeftSolid})if(!std::isfinite(x))throw std::runtime_error("nonfinite test trace scalar");};validate(f.world);for(const auto&c:f.candidates)validate(c.trace);return true;}
struct EnginePointEventsFixture {
    uint32_t world=0,partition=0,overflows=0;
    std::array<float,3>shortDelta{};
    std::vector<std::array<uint32_t,2>>calls;
};
inline void enginePointWriteOutput(std::ostream&out,const SourcePhysicsSearchTraceCandidate&t,const EnginePointEventsFixture&e){std::ostringstream bytes(std::ios::binary);searchWriteTrace(bytes,t);supportPut(bytes,e.world);supportPut(bytes,e.partition);supportPut(bytes,e.overflows);supportPut(bytes,e.shortDelta);supportPut(bytes,uint32_t(e.calls.size()));for(const auto&c:e.calls)supportPut(bytes,c);const auto data=bytes.str();supportPut(out,uint32_t(data.size()));out<<data;}
