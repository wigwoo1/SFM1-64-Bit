#pragma once
#include <cstdint>
struct GraphModifierSnapshotCandidate{bool bit25,bit26,bit27,left,middle;};
template<class Adapter>GraphModifierSnapshotCandidate graphModifierTransitionCandidate(std::uint32_t modifiers,std::uint32_t buttons,Adapter& a){
 GraphModifierSnapshotCandidate s{bool((modifiers>>25)&1),bool((modifiers>>26)&1),bool((modifiers>>27)&1),bool(buttons&1),bool((buttons>>2)&1)};
 if((a.state1c()&&!s.bit27)||(a.state1d()&&!s.bit26)||(a.state1e()&&!s.bit25))a.reset100d34e0();
 a.capturePointAfterReset();
 a.setMode174(s.bit27?(s.bit26?1:2):(s.bit26?3:0));
 return s;
}
