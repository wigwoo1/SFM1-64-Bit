#pragma once
#include "source_world_light_visibility_candidate.h"

// Recovered from the original static-prop manager's 101C4730. The handle is a
// 32-bit encoded value; it must not be confused with a native-width entity ID.
inline bool sourceStaticPropReferenceHandleCandidate(uint32_t handle){
    return (handle&0xffff0000u)==0x40000000u;
}
// Original manager 101C4700 regards a null entity as static without querying a
// handle. The caller supplies the entity's GetRefEHandle binding, not a cached
// classification or a guessed model-type flag.
template<class Services>
bool sourceStaticPropEntityCandidate(uintptr_t entity,Services&services){
    return entity==0||sourceStaticPropReferenceHandleCandidate(services.referenceHandle(entity));
}
// Original 101C4750 decodes low16, except FFFFFFFF maps to 1FFF. It does not
// establish that an index is live, in range, or belongs to a static prop.
inline uint32_t sourceStaticPropHandleIndexCandidate(uint32_t handle){
    return handle==UINT32_MAX?0x1fffu:handle&0xffffu;
}
template<class Services>
uint32_t sourceStaticPropEntityIndexCandidate(uintptr_t entity,Services&services){
    if(!entity)throw std::runtime_error("static-prop index requires an entity");
    return sourceStaticPropHandleIndexCandidate(services.referenceHandle(entity));
}
// Full 101943E0 for the light-query filter; world-only virtual 102727E0 returns
// false directly. The collision mask is deliberately unused by this predicate.
// Equality uses native-width identity; handle equality is not entity equality.
template<class Services>
bool sourceWorldLightShouldHitEntityCandidate(const SourceWorldLightFilterCandidate&filter,
    uintptr_t entity,uint32_t contentsMask,Services&services){
    (void)contentsMask;
    switch(filter.mode){
    case SourceWorldLightTraceModeCandidate::WorldOnly:return false;
    case SourceWorldLightTraceModeCandidate::EverythingFilterProps:
        if(!sourceStaticPropEntityCandidate(entity,services))return false;
        return entity!=filter.excludedEntity;
    default:throw std::runtime_error("unsupported world-light entity filter");
    }
}
