#pragma once
#include "buffer_read_check_candidate.h"
inline std::uint32_t controllerRegistrationHashCandidate(std::uint32_t code){
 const auto folded=code^((((code>>12)^code)>>12)&0xfffu);
 const auto high=static_cast<std::uint32_t>((std::uint64_t{folded}*0xaf286bcbULL)>>32);
 const auto quotient=(((folded-high)>>1)+high)>>4;
 return folded^((folded-quotient*19u)*13u);
}
inline std::int32_t controllerRegistrationCompareCandidate(std::uint32_t first,std::uint32_t second){return bufferSigned32Candidate(first-second);}
