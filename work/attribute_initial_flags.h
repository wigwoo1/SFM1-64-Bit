#pragma once
#include <cstdint>
// Ordinary constructor 0x103632C0 compares the complete supplied type word.
// Schema-specific flag additions and restore adjustments are separate steps.
inline uint16_t initialAttributeFlags(uint32_t suppliedType){
 uint16_t flags=static_cast<uint16_t>(suppliedType);
 if(suppliedType==1||suppliedType==15)flags|=0x400;
 return flags;
}
