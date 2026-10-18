#pragma once
#include <array>
#include <cstdint>
struct OwnerPreparationFixture{unsigned sourceLength,destinationLength,key;bool blocked;std::array<unsigned,32>keys;};
inline OwnerPreparationFixture ownerPreparationFixture(unsigned seed){OwnerPreparationFixture f{(seed/16)%17,(seed/512)%5,seed%4,(seed&8)!=0,{}};uint32_t r=seed+1;for(auto&key:f.keys){r=r*1664525u+1013904223u;key=(r>>16)%4;}return f;}
