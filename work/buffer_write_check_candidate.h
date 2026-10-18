#pragma once
#include "buffer_read_check_candidate.h"
template<class Adapter>
bool checkBufferWriteCandidate(std::uint32_t request,Adapter& a){
    if((a.errorByte()&1) || (a.flagsByte()&8))return false;
    const auto put=a.putPosition(),offset=a.bufferOffset();
    if(bufferSigned32Candidate(put)>=bufferSigned32Candidate(offset) &&
       bufferSigned32Candidate(a.capacity())>=bufferSigned32Candidate(put-offset+request))return true;
    if(a.writeOverflow(request))return true;
    a.setErrorByte(static_cast<std::uint8_t>(a.errorByte()|1));return false;
}
template<class Adapter>
void terminateBufferCandidate(Adapter& a){
    if(bufferSigned32Candidate(a.putPosition())<=bufferSigned32Candidate(a.maxGet()))return;
    if(!(a.flagsByte()&8) && !(a.errorByte()&1)){
        if(checkBufferWriteCandidate(1,a)){
            a.writeByte(a.putPosition()-a.bufferOffset(),0);
            a.setMaxGet(a.putPosition());return;
        }
        a.setErrorByte(static_cast<std::uint8_t>(a.errorByte()&~1u));
    }
    a.setMaxGet(a.putPosition());
}
