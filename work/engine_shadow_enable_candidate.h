#pragma once
#include <cstdint>
// Recovered1010A7D0. Explicit services keep unproved resource behavior external.
// Lookup runs after disable callbacks because native storage can change there.
template<class Cleanup,class Unlink,class Flags>
void engineShadowEnableCandidate(uint32_t handle,uint32_t enabled,
 Cleanup&& cleanup,Unlink&& unlink,Flags&& flags){
 if(uint8_t(enabled)==0){cleanup(handle);unlink(handle);}
 auto& word=flags(uint16_t(handle));
 if(uint8_t(enabled)!=0)word=uint16_t(word&0xfff7u);
 else word=uint16_t(word|8u);
}
// Owned representation of the node fields read by1010A6B0.
// Resource release and owner lookup remain explicit until their lifecycle is known.
struct EngineShadowCleanupNodeCandidate{
 uint16_t index=0;
 EngineShadowCleanupNodeCandidate* next=nullptr;
};
template<class Head,class Target,class Release>
void engineShadowCleanupCandidate(uint32_t handle,Head&& head,Target&& target,Release&& release){
 auto* node=head(uint16_t(handle));
 while(node){
  const uint16_t index=node->index;
  auto* next=node->next;
  release(target(index),index);
  node=next;
 }
 // Reload current record storage after callbacks, including empty traversal.
 head(uint16_t(handle))=nullptr;
}
