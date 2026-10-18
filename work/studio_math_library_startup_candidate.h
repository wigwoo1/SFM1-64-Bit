#pragma once
#include <cstdint>
struct StudioMathLibraryStartupCandidate {bool initialized=false;};
template<class Ops>void studioMathLibraryInitializeCandidate(StudioMathLibraryStartupCandidate& state,float gamma,float textureGamma,float brightness,int overbright,Ops& ops){
 if(state.initialized)return;
 auto flags=ops.cpuFlags();if(!(flags&8)||!(flags&16))ops.diagnostic("SSE and SSE2 are required.");
 state.initialized=true;ops.initializeTrigonometricTable();ops.buildGammaTables(gamma,textureGamma,brightness,overbright);ops.seedGenerator(0x31415926);
}
