#pragma once
#include "buffer_read_check_candidate.h"
inline bool defaultBufferReadOverflowCandidate(std::uint32_t){return false;}
template<class Adapter>
bool defaultBufferWriteOverflowCandidate(std::uint32_t request,Adapter& a){
    if(bufferSigned32Candidate(a.growSize())<0){
        if(!(a.flagsByte()&2))return false;
        a.convertExternalStorage(0);
    }
    while(bufferSigned32Candidate(a.capacity())<bufferSigned32Candidate(a.putPosition()-a.bufferOffset()+request))a.growStorage(1);
    return true;
}
