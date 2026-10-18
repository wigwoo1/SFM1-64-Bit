#pragma once
#include <cstdint>

inline bool ownerTraversalAttributeCandidate(std::uint32_t flags,
                                             std::uint32_t restriction) {
    if (restriction > 2) return false;
    if (restriction == 1 && (flags & 0x1000u)) return false;
    if (restriction == 2 && (!(flags & 0x800u) || (flags & 0x1000u))) return false;
    return true;
}
//10361a40 portable recursion. Adapter owns visited set, live reference storage,
// IsA and owner transition. The original hash/storage ABI remains unbound.
template<class Element,class Adapter>
void transitionElementOwnerGraphCandidate(Element* element,
    std::uint32_t newOwner,std::uint32_t restriction,std::uint32_t originalOwner,
    std::uint8_t sameOwnerRequired,Adapter& adapter) {
    if (sameOwnerRequired && adapter.owner(element) != originalOwner) return;
    const auto handle=adapter.handle(element);
    if (adapter.visited(handle)) return;
    adapter.markVisited(handle);
    adapter.transition(element,newOwner);
    for (auto* attribute=adapter.firstAttribute(element);attribute;
         attribute=adapter.nextAttribute(attribute)) {
        const auto flags=adapter.flags(attribute);
        if (!ownerTraversalAttributeCandidate(flags,restriction)) continue;
        const auto type=flags & 0x1fu;
        if (type == 1) {
            auto* child=adapter.resolve(adapter.scalarReference(attribute));
            if (child && adapter.isElement(child))
                transitionElementOwnerGraphCandidate(child,newOwner,restriction,
                    originalOwner,sameOwnerRequired,adapter);
        } else if (type == 15) {
            const auto count=adapter.arrayCount(attribute);
            for (std::int32_t i=0;i<count;++i) {
                auto* child=adapter.resolve(adapter.arrayReference(attribute,i));
                if (child && adapter.isElement(child))
                    transitionElementOwnerGraphCandidate(child,newOwner,restriction,
                        originalOwner,sameOwnerRequired,adapter);
            }
        }
    }
}
