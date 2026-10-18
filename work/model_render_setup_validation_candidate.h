#pragma once
#include <cstdint>
//100D8AE9..100D8B31 post-state after successful load. Original caller
// assumes four readable32-bit words and a valid descriptor; no size guard.
// Values are named by observed offsets, not inferred public file format.
inline uint16_t modelRenderSetupValidationCandidate(uint16_t flags,const uint32_t* loadedWords,uint32_t descriptorWord8,uint32_t configurationWord30){
 const bool matches=loadedWords[0]==2&&loadedWords[1]==descriptorWord8&&loadedWords[3]==configurationWord30*4u;
 return matches?uint16_t((flags&0xfffbu)|2u):uint16_t(flags|4u);
}
