#include <iostream>
#include "static_prop_registry_test_fixture.h"
#include "source_static_prop_point_spatial_candidate.h"
#include "physics_support_test_fixture.h"
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"owned x64");if(argc!=5)throw std::runtime_error("BSP mounts queries output required");
    StaticPropRegistryTestFixture fixture(argv[1],argv[2]);
    // Spatial preparation must not depend on fabricated/guessed surface IDs.
    for(auto&m:fixture.models)m.surfaceProperty.reset();
    SourceStaticPropPointSpatialCandidate scene(fixture.props,fixture.models,fixture.ids);
    std::ifstream in(argv[3],std::ios::binary);std::ofstream out(argv[4],std::ios::binary),
        states(std::string(argv[4])+".registrations",std::ios::binary),stored(std::string(argv[4])+".stored-boxes",std::ios::binary);
    if(!in||!out||!states||!stored)throw std::runtime_error("spatial composition streams");
    supportPut(out,uint32_t(scene.registeredCount()));
    for(size_t i=0;i<scene.size();++i){const auto&r=scene.registrationAt(i);supportPut(states,r.entityId);supportPut(states,r.diskSolidType);supportPut(states,r.effectiveSolidType);supportPut(states,uint32_t(r.partitionHandle));supportPut(states,uint32_t(bool(r.physicalBounds)));supportPut(states,r.physicalBounds.value_or(std::array<float,6>{}));if(r.physicalBounds)supportPut(out,r.partitionHandle);}
    for(size_t i=0;i<scene.registeredCount();++i){supportPut(stored,scene.partition().storedBounds(i));supportPut(stored,scene.partition().storedMask(i));supportPut(stored,scene.partition().storedFlags(i));}
    uint32_t count=0;supportGet(in,count);if(count>100000)throw std::runtime_error("spatial composition query extent");supportPut(out,count);
    for(uint32_t i=0;i<count;++i){SourceWorldLightRayCandidate ray;uint32_t mask=0,coarse=0,stop=0,swept=0;supportGet(in,mask);supportGet(in,ray.start);supportGet(in,ray.delta);supportGet(in,coarse);supportGet(in,stop);supportGet(in,swept);if(coarse||swept>1)throw std::runtime_error("unsupported spatial composition query");ray.swept=swept!=0;const auto answer=scene.enumerateCandidates(ray,mask,stop);supportPut(out,uint32_t(answer.size()));for(auto id:answer)supportPut(out,id);}
    if(in.peek()!=std::char_traits<char>::eof())throw std::runtime_error("spatial composition trailing queries");out.flush();states.flush();stored.flush();if(!out||!states||!stored)throw std::runtime_error("spatial composition output failure");
    std::cout<<"PASS owned spatial composition props="<<scene.size()<<" registered="<<scene.registeredCount()<<" queries="<<count<<" unresolved surfaces accepted for broadphase only\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
