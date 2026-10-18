#pragma once
#include <cstdint>
#include <vector>
// Base constructor 0x10361C93/97 preserves unowned high bits.
inline void initializeElementMutation(uint8_t&state,uint32_t&mask){state&=0xe0;mask&=0xfffffff0;}
// Native 0x1035DBD0 clears propagation bit and only the low two mask bits.
inline void clearElementPropagation(uint8_t&state,uint32_t&mask){state&=0xef;mask&=0xfffffffc;}
// Native 0x1035DBE0/0x1035DBF0 toggle only owner byte +0x3E bit 2.
inline void setElementNotifications(uint8_t&state,bool enabled){
 if(enabled)state|=4;else state&=0xfb;
}
// Native 0x1035DB90 queues the owner only on the first bit-0 transition.
template<class Queue> void markElementMutation(uint8_t&state,uint32_t handle,Queue queue){
 if(!(state&1))queue(handle);state|=3;
}
// Native 0x1035DBB0 clears owner bit 0 and each attached attribute's 0x80.
inline void clearElementMutation(uint8_t&state,std::vector<uint16_t>&attributeFlags){
 state&=0xfe;for(auto&flags:attributeFlags)flags&=0xff7f;
}
