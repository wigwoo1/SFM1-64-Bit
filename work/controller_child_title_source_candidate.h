#pragma once
#include <cstdint>
template<class Adapter>
auto controllerChildTitleSourceCandidate(Adapter& a) {
    if(!a.resolveOwner(a.childHandle80()))return a.modelSlot97(std::uint32_t{0xffffffff});
    auto* owner=a.resolveOwner(a.childHandle80());
    return a.modelSlot97(a.ownerField38(owner));
}
