#pragma once
#include "buffer_read_check_candidate.h"
template<class Adapter>
void readBufferByteCandidate(std::uint8_t* output,Adapter& a){
    if(!checkBufferReadCandidate(1,a)){*output=0;return;}
    *output=a.sourceByte(a.getPosition()-a.bufferOffset());
    a.setGetPosition(a.getPosition()+std::uint32_t{1});
}
