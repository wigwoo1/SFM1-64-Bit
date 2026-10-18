#include "source_world_light_entity_filter_candidate.h"
#include <iostream>
int main(){try{
    unsigned checks=0;auto require=[&](bool pass){if(!pass)throw std::runtime_error("entity filter assertion");++checks;};
    struct Services {unsigned calls=0;uint32_t handle=0x40000007u;uint32_t referenceHandle(uintptr_t){++calls;return handle;}}services;
    const uintptr_t a=0x123400001234ull,b=0x567800001234ull;SourceWorldLightFilterCandidate filter{SourceWorldLightTraceModeCandidate::EverythingFilterProps,a};
    require(sourceWorldLightShouldHitEntityCandidate(filter,b,0,services));require(!sourceWorldLightShouldHitEntityCandidate(filter,a,~0u,services));require(services.calls==2);
    require(sourceWorldLightShouldHitEntityCandidate(filter,0,0x4481,services));require(services.calls==2);filter.excludedEntity=0;require(!sourceWorldLightShouldHitEntityCandidate(filter,0,0,services));require(services.calls==2);
    filter.mode=SourceWorldLightTraceModeCandidate::WorldOnly;require(!sourceWorldLightShouldHitEntityCandidate(filter,b,0x4481,services));require(services.calls==2);
    filter.mode=SourceWorldLightTraceModeCandidate::EverythingFilterProps;services.handle=0xc0000007;require(!sourceWorldLightShouldHitEntityCandidate(filter,b,0x4481,services));require(services.calls==3);
    require(sourceStaticPropHandleIndexCandidate(UINT32_MAX)==0x1fff);require(sourceStaticPropHandleIndexCandidate(0x4000ffff)==0xffff);require(sourceStaticPropHandleIndexCandidate(0xc0000007)==7);
    bool rejected=false;try{sourceStaticPropEntityIndexCandidate(0,services);}catch(const std::runtime_error&){rejected=true;}require(rejected&&services.calls==3);
    filter.mode=SourceWorldLightTraceModeCandidate(2);rejected=false;try{sourceWorldLightShouldHitEntityCandidate(filter,b,0,services);}catch(const std::runtime_error&){rejected=true;}require(rejected&&services.calls==3);
    std::cout<<"PASS entity filter safety checks="<<checks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
