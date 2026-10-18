#pragma once
#include <cstdint>
// Local portion1035E400..1035E4C3. Return true when reverse references
// must be walked with (requested &~2). Queue callbacks may change state/flags.
template<class Queue,class CacheQueue>
bool dirtyRecoveredReferenceStateCandidate(uint8_t& state,uint32_t& flags,uint32_t requested,bool blocked,bool cachePresent,Queue queue,CacheQueue cacheQueue){
 if(blocked)return false;
 bool wasDirty=(state&0x10)!=0;
 if(wasDirty&&(((flags&3)|requested)==(flags&3)))return false;
 if(!wasDirty)queue();
 if(cachePresent){const uint32_t mask=(flags>>2)&3;bool covered=((mask&flags)==mask)&&(state&0x10);if(((flags>>2)&requested&3)==mask&&!covered)cacheQueue();}
 wasDirty=(state&0x10)!=0;state|=0x10;flags|=requested&3;
 const uint32_t propagated=requested&~2u;
 return !wasDirty||(((flags&3)|propagated)!=(flags&3));
}
