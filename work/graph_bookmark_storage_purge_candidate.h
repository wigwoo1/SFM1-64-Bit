#pragma once
template<class Adapter>void graphBookmarkStoragePurgeCandidate(Adapter& a){
 a.setCount0c(0);
 if(a.growSize8()>=0){if(auto* base=a.base0()){a.freeAllocatorSlot5(base);a.setBase0(nullptr);}a.setCapacity4(0);}
 a.setCachedBase10(a.base0());
}
