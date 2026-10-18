#pragma once
#include <cstdint>
struct StudioMaterialBufferCandidate {
 uintptr_t cursor=0,committedEnd=0,reservedEnd=0,base=0;
 bool dirty=false;uintptr_t pageAlignment=0;
};
//10073B20. Full-width arithmetic assumes representable sums and positive
// power-of-two page alignment. Allocation callbacks preserve the reserved base.
template<class Commit,class Decommit,class Warn,class OutOfMemory>
inline bool studioMaterialBufferResizeCandidate(StudioMaterialBufferCandidate& buffer,uintptr_t requestedEnd,Commit commit,Decommit decommit,Warn warn,OutOfMemory outOfMemory){
 auto align=[&](uintptr_t address){return (address+buffer.pageAlignment-1)&~(buffer.pageAlignment-1);};
 uintptr_t target=align(requestedEnd),old=buffer.committedEnd;
 if(target>buffer.reservedEnd)return false;
 if(target>old){if(old!=buffer.base&&buffer.dirty)buffer.dirty=false;uintptr_t additional=target-old;if(!commit(old,additional,0x1000,4)){outOfMemory(additional);return false;}buffer.committedEnd=target;if(target!=buffer.base)buffer.dirty=true;return true;}
 if(target==old)return true;
 if(buffer.cursor>target){warn();target=align(buffer.cursor);}
 old=buffer.committedEnd;if(target<old){if(buffer.dirty)buffer.dirty=false;decommit(target,old-target,0x4000);buffer.committedEnd=target;}return true;
}
