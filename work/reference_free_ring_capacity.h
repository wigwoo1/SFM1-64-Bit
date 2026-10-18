#pragma once
#include <cstdint>
#include <stdexcept>
//102BDBD0 normal positive-capacity range; negative grow size means external storage.
inline uint32_t recoveredReferenceStorageCapacityCandidate(uint32_t old,int32_t grow,uint32_t addition,uint32_t initial){
 if(grow<0)return old;
 const uint64_t needed=uint64_t(old)+addition;if(needed>0x7fffffffu)throw std::length_error("ring capacity overflow");
 uint64_t result;
 if(grow)result=((needed+uint32_t(grow)-1)/uint32_t(grow))*uint32_t(grow);
 else{result=old?old:initial;while(result<needed)result*=2;}
 if(result>0x7fffffffu)throw std::length_error("ring capacity overflow");
 return uint32_t(result);
}
inline uint32_t recoveredReferenceFreeRingCapacityCandidate(uint32_t old,int32_t grow,uint32_t addition){return recoveredReferenceStorageCapacityCandidate(old,grow,addition,8);}
//10353FE0 slots are8native bytes; default initial capacity4.
inline uint32_t recoveredReferenceSlotCapacityCandidate(uint32_t old,int32_t grow,uint32_t addition){return recoveredReferenceStorageCapacityCandidate(old,grow,addition,4);}
