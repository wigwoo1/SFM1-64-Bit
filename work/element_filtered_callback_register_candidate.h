#pragma once
#include <cstdint>
// 1035df40: mask bookkeeping continues even if node allocation fails.
template<class Node,class Callback,class Adapter>
inline void registerElementFilteredCallbackCandidate(Node*& head,Callback callback,
    std::uint32_t requiredMask,Adapter& a) {
    if(auto* node=a.allocateNode()) {
        node->callback=callback;node->mask=requiredMask;node->next=head;head=node;
    }
    const auto packed=a.packedMask();
    const auto prior=(packed>>2)&3u;
    const auto narrowed=prior&requiredMask;
    if((a.state()&0x10u) && ((packed&narrowed)&3u)==narrowed &&
       ((packed&(packed>>2))&3u)!=prior)a.enqueue();
    // Queue callback can alter unrelated bits; original rereads packed field.
    a.setPackedMask((a.packedMask()&~0xcu)|(narrowed<<2));
}
