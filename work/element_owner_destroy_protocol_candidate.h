#pragma once
#include <cstdint>
// Adapter resolution includes all original pool guards. Cleanup semantics and
// free implementation remain explicit; do not replace with guessed deletion.
template<class Adapter>
inline void destroyElementOwnerProtocolCandidate(std::uint32_t handle,Adapter& a) {
    auto* owner=a.resolve(handle);
    if(!owner)return;
    if(a.marked(owner))a.cleanupMarked(handle,1);
    if(a.rootServiceAvailable())a.cleanupRoot(a.root(owner),1);
    a.freeOwner(owner);
    a.releaseHandle(handle);
}
