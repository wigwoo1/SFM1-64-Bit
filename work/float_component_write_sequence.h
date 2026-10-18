#pragma once
#include "float_mutation.h"
#include <array>
#include <cstring>
// Quaternion 1036D270 type13 copies bits; vector3 10372DB0 uses x87 stores.
inline uint32_t recoveredX87FloatStoreBitsCandidate(uint32_t bits){if((bits&0x7f800000u)==0x7f800000u&&(bits&0x007fffffu))bits|=0x00400000u;return bits;}
template<size_t N,class Permission,class Undo,class Notify,class Finish>
bool writeRecoveredFloatComponentsCandidate(std::array<float,N>&storage,const std::array<float,N>&incoming,Permission permission,Undo undo,Notify notify,Finish finish){
 bool changed=false;for(size_t i=0;i<N;++i)if(floatMutationNeeded(storage[i],incoming[i])){changed=true;break;}if(!changed||!permission())return false;undo();std::memcpy(storage.data(),incoming.data(),N*4);if constexpr(N==3)for(size_t i=0;i<N;++i){uint32_t bits;std::memcpy(&bits,&storage[i],4);bits=recoveredX87FloatStoreBitsCandidate(bits);std::memcpy(&storage[i],&bits,4);}notify();finish();return true;
}
