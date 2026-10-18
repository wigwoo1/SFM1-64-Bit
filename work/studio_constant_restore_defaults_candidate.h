#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
inline uint32_t studioConstantRestoreReciprocalBitsCandidate(uint32_t bits){
 float value;std::memcpy(&value,&bits,4);
 const __m128 result=_mm_div_ss(_mm_set_ss(1.0f),_mm_set_ss(value));
 const float scalar=_mm_cvtss_f32(result);uint32_t output;std::memcpy(&output,&scalar,4);return output;
}
template<class Backend>
void studioConstantRestoreDefaultsCandidate(uint32_t inputBits,Backend& backend){
 const auto publish=[&](unsigned index,const std::array<uint32_t,4>& words){
  if(std::memcmp(backend.primaryCache()+index*4,words.data(),16)==0)return;
  backend.submitVertex(index,words);
  // Submission can replace either cache; reload both after the callback.
  std::memmove(backend.primaryCache()+index*4,words.data(),16);
  std::memmove(backend.secondaryCache()+index*4,words.data(),16);
 };
 publish(0,{0,0x3f800000,0x40000000,0x3f000000});
 publish(1,{0x3ee8ba2e,inputBits,0x3eaaaaab,studioConstantRestoreReciprocalBitsCandidate(inputBits)});
 publish(3,{0x41d80000,0x42000000,0x443f40a4,0x42680000});
 publish(225,{0,0,0,0});
}
