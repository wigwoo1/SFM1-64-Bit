#pragma once
#include <cstdint>
struct StudioShaderSnapshotSelectionStateCandidate {uintptr_t shadow=0,currentApi=0;};
// Logical100653c0. Ops supplies live parameter2 flags and API callbacks.
// Calls are serialized, matching native shared globals. No public ABI claim.
template<class Ops>uint32_t studioShaderSnapshotSelectionCandidate(StudioShaderSnapshotSelectionStateCandidate& state,uintptr_t api,Ops& ops){
 uint32_t result=0;state.currentApi=api;
 uint32_t first=state.shadow?((ops.flags()>>10)&1):ops.query17(api);
 if(uint8_t(first))result=2;
 uint32_t second=state.shadow?((ops.flags()>>13)&1):ops.query19(state.currentApi);
 if(uint8_t(second))result|=16;
 if(ops.query63(api)>=1)result|=4;
 auto flags=ops.flags();if((flags&0xa00u)==0xa00u)result|=8;
 bool final;
 if(state.shadow){if(flags&0x100000u)result|=32;final=(flags&0x200000u)!=0;}
 else{auto bits=ops.query29(api,0);if(bits&1)result|=32;final=(bits&2)!=0;}
 state.currentApi=0;if(final)result|=64;
 return result;
}
