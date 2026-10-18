#pragma once
#include <cstdint>
// Capture receiver in caller. Ops must reload slot10 binding after slot9.
template<class Owner,class Ops>
void studioShaderManagerRestoreResetCandidate(Owner& owner,Ops& ops){
 auto slot9=ops.captureSlot9(owner);
 owner.cached9c=0xffffffff;owner.cachedA0=0xffffffff;
 ops.invokeSlot9(slot9,owner,0xffffffff);
 ops.invokeSlot10(owner,0xffffffff);
}
