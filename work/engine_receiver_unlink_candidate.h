#pragma once
#include <cstdint>
struct EngineReceiverLinkCandidate {uint16_t previous=0xffff,next=0xffff;};
struct EngineReceiverPoolCandidate {
 EngineReceiverLinkCandidate* links=nullptr;
 int32_t capacity=0,highwater=-1;
 uint32_t head=0xffff,tail=0xffff,count=0;
};
//10107F60 full32-bit validation and32-bit count; validneighbors required.
inline void engineReceiverUnlinkCandidate(EngineReceiverPoolCandidate& pool,int32_t index){
 if(index<0||index>=pool.capacity||index>pool.highwater)return;
 auto& node=pool.links[index];if(int32_t(node.previous)==index)return;
 auto previous=node.previous,next=node.next;
 if(previous==0xffff)pool.head=next;else pool.links[previous].next=next;
 if(next==0xffff)pool.tail=previous;else pool.links[next].previous=previous;
 --pool.count;node.previous=uint16_t(index);node.next=uint16_t(index);
}
