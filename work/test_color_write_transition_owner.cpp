#include "native_d3d9_fill_transition_candidate.h"
#include <cstdio>
int main(){
 unsigned cases=0;
 for(uint32_t value=0;value<65536;++value)for(bool replace:{false,true}){
  NativeD3d9TransitionStateCandidate state{};
  const uint32_t initial=value|0xabcd0000u,updated=initial^0x13579bdfu,later=~updated;
  std::memcpy(state.data()+8,&initial,4);
  uint32_t first=0x11111111,second=0x22222222;
  uint32_t* live=&first;unsigned calls=0,lookups=0;
  nativeD3d9ColorWriteTransitionOwnerCandidate(state,[&](D3DRENDERSTATETYPE key,DWORD submitted){
   if(key!=D3DRS_COLORWRITEENABLE||submitted!=initial)throw 1;
   ++calls;std::memcpy(state.data()+8,&updated,4);if(replace)live=&second;
   return E_FAIL; // Native cache publication follows even a failing HRESULT.
  },[&]() ->uint32_t& {
   ++lookups;if(calls!=1)throw 2;std::memcpy(state.data()+8,&later,4);return *live;
  });
  if(calls!=1||lookups!=1||*live!=updated)return 3;
  if(replace?first!=0x11111111:second!=0x22222222)return 4;
  ++cases;
 }
 std::printf("color_write_owner_cases=%u live_owner_reload=1 state_reload_before_owner_lookup=1 failed_hresult_cache_publication=1\n",cases);
 return 0;
}
