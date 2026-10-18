#pragma once
#include <array>
#include <cstdint>
inline bool studioCpuIdMasksCandidate(uint8_t total,uint8_t cores,std::array<uint8_t,8>& output){
 // Native divides by min(cores,total); zero inputs cause division fault.
 if(!total||!cores)return false;
 if(cores>total)cores=total;
 auto width=[](unsigned value){unsigned bits=0;while(value){++bits;value>>=1;}return bits;};
 const unsigned threads=width(total/cores-1u),coreBits=width(cores-1u);
 const uint8_t upper=static_cast<uint8_t>(0xffu<<(threads+coreBits));
 const uint8_t lower=static_cast<uint8_t>(0xffu<<threads);
 output={total,cores,static_cast<uint8_t>(threads),static_cast<uint8_t>(~lower),static_cast<uint8_t>(coreBits),static_cast<uint8_t>(lower^upper),static_cast<uint8_t>(8u-coreBits-threads),upper};return true;
}
