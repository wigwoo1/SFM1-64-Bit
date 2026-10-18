#include <future>
#include <iostream>
#include <memory>
#include "source_displacement_trace_surface_properties_candidate.h"
int main(){try{
    static_assert(sizeof(void*)==8,"owned x64");unsigned checks=0;auto require=[&](bool ok){if(!ok)throw std::runtime_error("displacement property assertion");++checks;};
    std::vector<std::array<uint16_t,2>>input{{0,65535},{17,42},{65535,0}};auto binding=std::make_shared<const SourceDisplacementTraceSurfacePropertiesCandidate>(input);
    for(uint32_t i=0;i<3;++i)for(uint16_t flags:{uint16_t(0),uint16_t(15),uint16_t(16),uint16_t(17),uint16_t(23),uint16_t(65535)})require(binding->property({SourceCollisionSurfaceKindCandidate::Displacement,i},flags)==input[i][(flags&16)?1:0]);
    auto reject=[&](auto&&fn){bool caught=false;try{fn();}catch(const std::exception&){caught=true;}require(caught);require(binding->size()==3&&binding->property({SourceCollisionSurfaceKindCandidate::Displacement,1},16)==42);};
    for(auto kind:{SourceCollisionSurfaceKindCandidate::Default,SourceCollisionSurfaceKindCandidate::TextureInfo,SourceCollisionSurfaceKindCandidate::RuntimeSurface,SourceCollisionSurfaceKindCandidate::BoxFace})reject([&](){binding->property({kind,1},16);});
    for(uint32_t i:{3u,65535u,UINT32_MAX})reject([&](){binding->property({SourceCollisionSurfaceKindCandidate::Displacement,i},16);});
    reject([&](){binding=std::make_shared<const SourceDisplacementTraceSurfacePropertiesCandidate>(std::vector<std::array<uint16_t,2>>(65536));});
    const SourceDisplacementTraceSurfacePropertiesCandidate empty({});reject([&](){empty.property({SourceCollisionSurfaceKindCandidate::Displacement,0},0);});
    input.clear();input.shrink_to_fit();require(binding->property({SourceCollisionSurfaceKindCandidate::Displacement,0},16)==65535&&binding->property({SourceCollisionSurfaceKindCandidate::Displacement,2},16)==0);
    std::array<std::future<bool>,4>jobs;for(auto&job:jobs)job=std::async(std::launch::async,[&](){for(unsigned i=0;i<256;++i)if(binding->property({SourceCollisionSurfaceKindCandidate::Displacement,1},uint16_t(i))!=((i&16)?42:17))return false;return true;});for(auto&job:jobs)require(job.get());
    std::cout<<"PASS displacement surface binding safety checks="<<checks<<" concurrent_lookups=1024 no implicit unresolved/default substitution\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
