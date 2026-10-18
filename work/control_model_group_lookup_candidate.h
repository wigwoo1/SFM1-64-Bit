#pragma once
#include <cstdint>
// Recovered1008d2d0: explicit callbacks avoid assuming original32-bit offsets.
template<class Element, class Resolve, class IsGroup, class IsAnimationSet, class GetGroup>
Element* controlModelGroupLookupCandidate(std::int32_t kind,std::uint32_t handle,
    Resolve resolve,IsGroup isGroup,IsAnimationSet isAnimationSet,GetGroup getGroup) {
    if (kind != 2 && kind != 3) return nullptr;
    auto element = resolve(handle);
    if (!element) return nullptr;
    if (kind == 3) return isGroup(element) ? element : nullptr;
    if (!isAnimationSet(element)) return nullptr;
    return getGroup(element);
}
