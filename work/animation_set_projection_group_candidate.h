#pragma once
#include <type_traits>
#include "control_model_group_lookup_candidate.h"
#include "control_model_item_flags_candidate.h"
// Uses current owned reference payload, not stale serialized DMX indices.
// Type callbacks must implement original IsA semantics including subclasses.
template<class Projection,class IsGroup,class IsAnimationSet>
auto animationSetProjectionGroupCandidate(Projection& projection,std::int32_t kind,
    std::uint32_t handle,IsGroup isGroup,IsAnimationSet isAnimationSet)
    -> decltype(projection.scene.resolve(handle)) {
    using Element = typename std::remove_pointer<decltype(projection.scene.resolve(handle))>::type;
    return controlModelGroupLookupCandidate<Element>(kind,handle,
        [&](std::uint32_t h){return projection.scene.resolve(h);},isGroup,isAnimationSet,
        [&](Element* owner)->Element* {
            for (auto* attribute=owner->attributes;attribute;attribute=attribute->next) {
                const auto found=projection.attributeLocations.find(attribute->token);
                if (found==projection.attributeLocations.end()) continue;
                const auto location=found->second;
                const auto& source=projection.graph.elements.at(location.first).attributes.at(location.second);
                if (projection.graph.strings.at(source.name)!="rootControlGroup") continue;
                if ((attribute->flags&31)!=1 || attribute->references().size()!=1) return nullptr;
                return projection.scene.resolve(attribute->references()[0]);
            }
            return nullptr;
        });
}
// Original column/kind branches remain lazy: group lookup occurs only in column3.
template<class Projection,class IsGroup,class IsAnimationSet,class IsTransform>
std::uint32_t animationSetProjectionFlagsCandidate(Projection& projection,
    std::uint32_t base,bool nodePresent,std::int32_t kind,std::uint32_t handle,
    std::int32_t column,IsGroup isGroup,IsAnimationSet isAnimationSet,IsTransform isTransform) {
    return controlModelItemFlagsCandidate(base,nodePresent,column,kind,
        [&](){return animationSetProjectionGroupCandidate(projection,kind,handle,isGroup,isAnimationSet)!=nullptr;},
        isTransform);
}
// Model identity is supplied by the UI bridge; no guessed Qt private layout.
template<class Projection,class IsGroup,class IsAnimationSet,class IsTransform>
std::uint32_t animationSetProjectionIndexFlagsCandidate(Projection& projection,
    std::int32_t row,std::int32_t column,bool modelIdentityMatches,bool nodePresent,
    std::int32_t kind,std::uint32_t handle,IsGroup isGroup,
    IsAnimationSet isAnimationSet,IsTransform isTransform) {
    return animationSetProjectionFlagsCandidate(projection,
        controlModelBaseFlagsCandidate(row,column,modelIdentityMatches),nodePresent,
        kind,handle,column,isGroup,isAnimationSet,isTransform);
}
