#pragma once
#include <cstdint>
// Engine 10143990: state>=2 reads CNetworkSystem slot43 (float at +30h).
// x87 FCOMIP zero,value; JB permits positive and unordered values.
// Raw classification preserves this branch without quieting signaling NaNs.
// Floating-point exception flags/control modes are outside this candidate.
inline bool clockCorrectionGateCandidate(int32_t state,uint32_t networkWord,int32_t enabled){
 if(state>=2){
  const uint32_t magnitude=networkWord&0x7fffffffu;
  const bool unordered=magnitude>0x7f800000u;
  const bool positive=(networkWord&0x80000000u)==0&&magnitude!=0;
  if(!unordered&&!positive)return false;
 }
 return enabled!=0;
}
