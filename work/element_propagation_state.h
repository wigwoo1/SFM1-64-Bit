#pragma once
#include <cstdint>
struct ElementPropagationDecision{bool changed=false,queuePropagated=false,queuePrepared=false,traverseIncoming=false;uint32_t incomingMask=0;};
// Owner-local portion of 0x1035E400. Caller performs queue insertion and
// incoming-reference traversal, skipping attributes with flag 0x1000.
inline ElementPropagationDecision beginElementPropagation(uint8_t&state,uint32_t&mask,uint32_t requested,bool suppressed,bool hasGeneralCallbacks){
 ElementPropagationDecision result;
 if(suppressed)return result;
 bool active=(state&0x10)!=0;auto old=mask&3;
 if(active&&(old|requested)==old)return result;
 result.changed=true;result.queuePropagated=!active;
 auto required=(mask>>2)&3;
 bool previouslyMatched=active&&((old&required)==required);
 result.queuePrepared=hasGeneralCallbacks&&((requested&required)==required)&&!previouslyMatched;
 state|=0x10;mask|=requested&3;
 result.incomingMask=requested&~uint32_t(2);
 result.traverseIncoming=!active||(((mask&3)|result.incomingMask)!=(mask&3));
 return result;
}
