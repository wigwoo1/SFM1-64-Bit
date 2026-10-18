#pragma once
#include <cstdint>
// 10362EB0/10362EF0 re-read gates independently at each invocation.
template<class Notify>
void recoveredFloatArrayOwnerNotificationCandidate(uint16_t attributeFlags,
 uint8_t ownerFlags,int32_t first,int32_t last,Notify notify){
 if((attributeFlags&0x100)&&(ownerFlags&4))notify(first,last);
}
