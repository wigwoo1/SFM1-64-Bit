#pragma once
#include <cstdint>
// Owned links corresponding to101083F0/101084B0. Valid neighboring indices
// and sufficient storage are caller preconditions, as in the recovered methods.
struct EngineShadowPoolLinkCandidate{uint16_t previous=0xffff,next=0xffff;};
struct EngineShadowPoolStateCandidate{
 EngineShadowPoolLinkCandidate* links=nullptr;
 int32_t capacity=0;
 uint16_t head=0xffff,tail=0xffff,count=0,maximumIndex=0;
};
inline void engineShadowPoolUnlinkCandidate(EngineShadowPoolStateCandidate& pool,uint32_t argument){
 const uint16_t index=uint16_t(argument);
 if(int32_t(index)>=pool.capacity||index>pool.maximumIndex)return;
 auto& node=pool.links[index];
 if(node.previous==index)return;
 const auto previous=node.previous,next=node.next;
 if(previous==0xffff)pool.head=next;else pool.links[previous].next=next;
 if(next==0xffff)pool.tail=previous;else pool.links[next].previous=previous;
 node.previous=index;node.next=index;pool.count=uint16_t(pool.count-1u);
}
