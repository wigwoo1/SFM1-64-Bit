#pragma once
#include <cstdint>
//1035F900: temporarily disable model mode, destroy attributes head-to-tail,
// mark dirty, set frameworkflag1000, then restore original low-byte mode.
template<class Attribute,class GetMode,class SetMode,class Next,class Destroy,class Dirty>
void finalizeRecoveredReferenceElementCandidate(Attribute*& head,uint32_t& frameworkFlags,GetMode getMode,SetMode setMode,Next next,Destroy destroy,Dirty dirty){
 const uint8_t saved=uint8_t(getMode());setMode(0);
 while(head){auto* attribute=head;auto* following=next(attribute);destroy(attribute);head=following;}
 dirty(3);frameworkFlags|=0x1000;setMode(saved);
}
