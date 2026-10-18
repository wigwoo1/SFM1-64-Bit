#pragma once
#include <cstdint>
// Native uint32 nesting is not atomic; caller must serialize this context.
template<class Enqueue> void studioMaterialBeginCandidate(uint32_t& depth,Enqueue enqueue){if(++depth==1)enqueue();}
template<class Enqueue> void studioMaterialEndCandidate(uint32_t& depth,Enqueue enqueue){if(--depth==0)enqueue();}
