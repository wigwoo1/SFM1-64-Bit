#pragma once
#include <cstdint>
template<class Item,class Adapter>Item* insertControllerContainerCandidate(Item* item,Adapter& a){
 const auto ratio=(a.itemCount()<<8)/a.activeBuckets();a.setLookupError(0);
 if(a.growThreshold()<=ratio)a.resizeContainer();
 std::uint32_t hash;auto** slot=a.searchItemSlot(item,&hash);
 if(auto* node=*slot){auto* previous=node->data;node->data=item;a.setReplacements(a.replacements()+1);return previous;}
 auto* node=a.allocateNode();
 if(!node){a.setLookupError(a.lookupError()+1);return nullptr;}
 node->hash=hash;node->data=item;node->next=nullptr;*slot=node;
 a.setInsertions(a.insertions()+1);a.setItemCount(a.itemCount()+1);return nullptr;
}
