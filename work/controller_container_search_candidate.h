#pragma once
#include <cstdint>
template<class Adapter>auto searchControllerContainerCandidate(const std::uint32_t* key,std::uint32_t* output,Adapter& a){
 const auto hash=a.hashKey(key);a.setHashCalls(a.hashCalls()+1);*output=hash;
 auto bucket=hash%a.divisor18();if(bucket<a.split14())bucket=hash%a.divisor10();
 const auto compare=a.compareCallback();
 auto** slot=a.bucketSlot(bucket);
 while(auto* node=*slot){
  a.setVisited(a.visited()+1);
  if(node->hash==hash){a.setComparisons(a.comparisons()+1);if(compare(node->data,key)==0)return slot;}
  slot=&node->next;
 }
 return slot;
}
