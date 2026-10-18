#pragma once
#include <cstdint>
// Whole1035B220 mode0 orchestration; physical destruction supplied by factory.
// Incoming iteration occurs before resolving/validating the element handle.
template<class Phase,class Incoming,class Resolve,class Observe,class Dirty,class Erase,class BeforeDestroy,class Finalize,class Destroy,class Retire>
bool removeRecoveredReferenceElementModeZeroCandidate(uint32_t handle,uint32_t& frameworkFlags,Phase phase,Incoming incoming,Resolve resolve,Observe observe,Dirty dirty,Erase erase,BeforeDestroy beforeDestroy,Finalize finalize,Destroy destroy,Retire retire){
 auto p=phase();if((p!=0&&p!=1)||handle==0xffffffffu)return false;
 incoming(handle);auto*e=resolve(handle);if(!e)return false;
 observe(*e);dirty(*e,true);erase(*e);beforeDestroy(*e);finalize(*e);destroy(handle);retire(handle);frameworkFlags|=0x1000;return true;
}
