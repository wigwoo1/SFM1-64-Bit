#pragma once
#include <cstdint>
// Logical x64 fields recovered from 10008BB3..10008CC1; native byte offsets
// are not an x64 ABI. getNode reloads pool storage after unlinkIndex.
struct StudioDecalChainBufferCandidate {
 uintptr_t address=0; int32_t capacity=0,growth=0;
};
struct StudioDecalChainNodeCandidate {
 StudioDecalChainBufferCandidate array, list48, list24;
 int32_t arrayCount=0; uintptr_t arrayCache=0;
 uint16_t contribution=0,next=0xffff,previous=0xffff;
};
template<class GetNode,class Unlink,class Remove24,class Remove48,class Free>
inline void studioDecalChainReleaseCandidate(uint16_t head,uint16_t& freeHead,uint32_t& counter,GetNode getNode,Unlink unlinkIndex,Remove24 remove24,Remove48 remove48,Free freeBuffer){
 while(head!=0xffff){
  auto* before=getNode(head);const uint16_t next=before->next;
  counter-=before->contribution;unlinkIndex(head);
  auto* node=getNode(head);
  remove24(node->list24);
  if(node->list24.growth>=0){if(node->list24.address){freeBuffer(node->list24.address);node->list24.address=0;}node->list24.capacity=0;}
  remove48(node->list48);
  if(node->list48.growth>=0){if(node->list48.address){freeBuffer(node->list48.address);node->list48.address=0;}node->list48.capacity=0;}
  node->arrayCount=0;
  if(node->array.growth>=0){if(node->array.address){freeBuffer(node->array.address);node->array.address=0;}node->array.capacity=0;}
  node->arrayCache=node->array.address;
  // Preserve the original second ownership check: freeBuffer may change growth.
  if(node->array.growth>=0){if(node->array.address){freeBuffer(node->array.address);node->array.address=0;}node->array.capacity=0;}
  node->next=freeHead;freeHead=head;head=next;
 }
}
