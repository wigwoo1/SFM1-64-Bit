#pragma once
#include "buffer_read_check_candidate.h"
template<class Adapter,class Item>
void disposeControllerCollectionItemCandidate(Item* item,Adapter& a){
 if(!item)return;
 while(a.itemCount(item)>0)a.removeItem3f80(item,a.itemField4(item));
 a.releasePointer(item);
}
template<class Adapter>
void disposeControllerCollectionCandidate(Adapter& a){
 for(std::uint32_t i=0;bufferSigned32Candidate(i)<bufferSigned32Candidate(a.collectionCount());++i){
  disposeControllerCollectionItemCandidate(a.collectionItem(i),a);
  a.clearCollectionItem(i);
 }
 a.releasePointer(a.collectionStorage());
}
