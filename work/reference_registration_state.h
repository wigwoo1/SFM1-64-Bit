#pragma once
#include <cstdint>
// Element+3C context mask, +3E state byte. Preserve x86 shift count masking.
struct RecoveredReferenceRegistrationStateCandidate {
 uint16_t contexts=0;uint8_t state=0;
 bool suppressed()const{return(state&8)!=0;}
 bool eligible(uint32_t context)const{return(uint32_t(contexts)&(1u<<(context&31)))!=0;}
 void setEligible(bool enabled,uint32_t context){const auto bit=1u<<(context&31);contexts=uint16_t(enabled?(uint32_t(contexts)|bit):(uint32_t(contexts)&~bit));}
};
