#pragma once
#include <cstdint>
template<class Adapter>void resizeControllerContainerCandidate(Adapter& a){
 const auto split=a.split14(),mod=a.divisor18(),capacity=a.divisor10();
 a.setSplit14(split+1);
 if(split+1>=mod){
  const auto doubled=capacity+capacity;auto** base=a.reallocateBuckets(doubled);
  if(!base){a.setLookupError(a.lookupError()+1);a.setSplit14(0);return;}
  for(auto i=a.divisor10();i<doubled;++i)base[i]=nullptr;
  a.setDivisor18(a.divisor10());a.setDoublings(a.doublings()+1);a.setDivisor10(doubled);a.setSplit14(0);a.setBucketStorage(base);
 }
 a.setActiveBuckets(a.activeBuckets()+1);a.setSplits(a.splits()+1);
 auto** source=a.bucketSlot(split);auto** target=a.bucketSlot(split+mod);*target=nullptr;
 while(auto* node=*source){
  if(node->hash%capacity==split)source=&node->next;
  else{*source=node->next;node->next=*target;*target=node;}
 }
}
