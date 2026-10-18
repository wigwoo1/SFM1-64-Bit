#include "source_spatial_point_ray_adapter_candidate.h"
#include "source_engine_point_trace_candidate.h"
#include <iostream>
struct AdapterServices {
    const SourceSpatialPointPartitionCandidate&partition;
    std::vector<uint64_t>candidates;
    bool swept=false;
    std::array<float,3>delta{};
    unsigned clipCalls=0;
    SourcePhysicsSearchTraceCandidate traceWorld(const SourceWorldLightRayCandidate&,uint32_t){SourcePhysicsSearchTraceCandidate t;t.fraction=0;return t;}
    std::vector<uint64_t>enumerateCandidates(const SourceWorldLightRayCandidate&ray){swept=ray.swept;delta=ray.delta;return candidates=sourceSpatialPointEnumerateRayCandidate(partition,ray,0x65);}
    void candidateOverflow(size_t){throw std::runtime_error("unexpected fixture overflow");}
    uintptr_t resolveCollideable(uint64_t id){return uintptr_t(id);}
    uint32_t referenceHandle(uintptr_t){return 0x40000001;}
    SourcePhysicsSearchTraceCandidate clipToCollideable(const SourceWorldLightRayCandidate&,uint32_t,uintptr_t){++clipCalls;return {};}
};
int main(){try{static_assert(sizeof(void*)==8,"owned x64");unsigned checks=0;auto require=[&](bool ok){if(!ok)throw std::runtime_error("ray adapter assertion");++checks;};const uint64_t center=0x11223344decafbad,edge=0x55667788decafbad;const SourceSpatialPointPartitionCandidate partition({{center,0x65,{-4,-4,-4,4,4,4}},{edge,0x65,{16383,-4,-4,16384,4,4}}});
    require(sourceSpatialPointEnumerateRayCandidate(partition,{{20000,0,0},{0,0,0},true},0x65).empty());require(sourceSpatialPointEnumerateRayCandidate(partition,{{20000,0,0},{0,0,0},false},0x65)==std::vector<uint64_t>{edge});
    require(sourceSpatialPointEnumerateRayCandidate(partition,{{-100,0,0},{200,0,0},false},0x65).empty());require(sourceSpatialPointEnumerateRayCandidate(partition,{{-100,0,0},{200,0,0},true},0x65)==std::vector<uint64_t>{center});
    for(bool flag:{false,true}){AdapterServices services{partition,{},false,{},0};const SourceWorldLightRayCandidate ray{{20000,0,0},{-4000,0,0},flag};sourceEnginePointTraceCandidate(ray,1,{SourceWorldLightTraceModeCandidate::EverythingFilterProps,0},services);require(services.swept==flag);require(services.delta==std::array<float,3>{0,0,0});require(flag?services.candidates.empty():services.candidates==std::vector<uint64_t>{edge});require(services.clipCalls==(flag?0u:1u));}
    const auto baseline=partition.queryPoint({0,0,0},{0,0,0},0x65);for(bool flag:{false,true}){bool caught=false;try{sourceSpatialPointEnumerateRayCandidate(partition,{{std::numeric_limits<float>::infinity(),0,0},{0,0,0},flag},0x65);}catch(const std::exception&){caught=true;}require(caught);require(partition.queryPoint({0,0,0},{0,0,0},0x65)==baseline);}
    std::cout<<"PASS explicit ray adapter checks="<<checks<<" zero-fraction caller integration preserved\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
