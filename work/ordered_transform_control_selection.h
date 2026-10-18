#pragma once
#include <cstdint>
// Mirrors104C65C0 once104C6510 has produced its ordered channel candidates.
// Type predicate must implement runtime class membership, including subclasses.
template<class OrderedChannels,class GetTransform,class IsControl>
uint32_t selectRecoveredOrderedTransformControlCandidate(const OrderedChannels&channels,GetTransform getTransform,IsControl isControl){
 for(uint32_t channel:channels){if(channel==0xffffffffu)continue;const uint32_t transform=getTransform(channel);
  if(transform!=0xffffffffu&&isControl(transform))return transform;}
 return 0xffffffffu;
}
