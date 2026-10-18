#pragma once
#include "reference_dirty_state.h"
// Reverse enumeration supplies attributes in native iterator order.
template<class Element,class Blocked,class Queue,class CacheQueue,class Reverse>
void dirtyRecoveredReferenceGraphCandidate(Element* e,uint32_t request,Blocked blocked,Queue queue,CacheQueue cacheQueue,Reverse reverse){
 if(!dirtyRecoveredReferenceStateCandidate(e->state,e->flags,request,blocked(),e->cachePresent,[&](){queue(e);},[&](){cacheQueue(e);}))return;
 reverse(e,[&](uint16_t flags,Element* owner){if(!(flags&0x1000)&&owner)dirtyRecoveredReferenceGraphCandidate(owner,request&~2u,blocked,queue,cacheQueue,reverse);});
}
