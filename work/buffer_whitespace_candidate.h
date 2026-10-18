#pragma once
#include "buffer_read_check_candidate.h"
template<class Adapter>
void skipBufferWhitespaceCandidate(Adapter& a){
    if(!(a.flagsByte()&1) || a.errorByte()!=0)return;
    while(checkBufferReadCandidate(1,a)){
        const auto get=a.getPosition();
        const auto ch=a.sourceByte(get-a.bufferOffset());
        if(!((ch>=9 && ch<=13) || ch==32))return;
        a.setGetPosition(get+1);
    }
}
