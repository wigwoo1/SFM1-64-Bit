#pragma once
#include <cstdint>
#include <initializer_list>
// Original10360DC0; callbacks expose still-unrecovered teardown/cache operations.
template<class Before,class Queue,class Dirty,class Clear>
bool updateRecoveredReferenceRegistrationCandidate(uint8_t& state,uint32_t& flags,uint32_t enabled,Before before,Queue queue,Dirty dirty,Clear clear){
 const uint8_t requested=uint8_t(enabled);if(requested==uint8_t((state>>3)&1))return false;
 if(requested)before();state=uint8_t((state&~8u)|((requested<<3)&8));
 if(!(state&1))queue();state|=3;dirty(3);
 for(uint32_t offset:{0x44u,0x48u,0x4cu,0x54u,0x58u,0x50u})clear(offset);
 flags&=0xfffffff0u;dirty(3);return true;
}
