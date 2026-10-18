#pragma once
#include "buffer_read_check_candidate.h"
inline bool bufferWhitespaceByteCandidate(std::uint8_t ch){return (ch>=9 && ch<=13)||ch==32;}
template<class Adapter>
std::uint32_t countBufferWhitespaceCandidate(std::uint32_t look,Adapter& a){
    if(!(a.flagsByte()&1) || a.errorByte()!=0)return 0;
    for(;;){
        if(a.errorByte()&2)return look;
        const bool ok=checkBufferReadCandidate(look+1,a);
        a.setErrorByte(static_cast<std::uint8_t>(a.errorByte()&~2u));
        if(!ok)return look;
        const auto ch=a.sourceByte(a.getPosition()-a.bufferOffset()+look);
        if(!bufferWhitespaceByteCandidate(ch))return look;
        ++look;
    }
}
template<class Adapter>
std::uint32_t bufferStringLengthCandidate(Adapter& a){
    if(a.errorByte()!=0)return 0;
    std::uint32_t look=(a.flagsByte()&1)?countBufferWhitespaceCandidate(0,a):0;
    const auto start=look;
    for(;;){
        const auto get=a.getPosition();const auto max=a.maxGet();
        if(bufferSigned32Candidate(get+look)>=bufferSigned32Candidate(max))break;
        std::uint32_t chunk=128;
        if(bufferSigned32Candidate(get+look+chunk)>bufferSigned32Candidate(max))chunk=max-get-look;
        if(!(a.errorByte()&2)){
            checkBufferReadCandidate(look+chunk,a);
            a.setErrorByte(static_cast<std::uint8_t>(a.errorByte()&~2u));
        }
        const auto remaining=a.maxGet()-a.getPosition();
        if(bufferSigned32Candidate(remaining)<bufferSigned32Candidate(chunk))chunk=remaining;
        if(!chunk)break;
        const auto* source=a.sourceData()+(a.getPosition()-a.bufferOffset()+look);
        const bool text=(a.flagsByte()&1)!=0;
        for(std::uint32_t i=0;bufferSigned32Candidate(i)<bufferSigned32Candidate(chunk);++i){
            const auto ch=source[i];
            if(ch==0 || (text && bufferWhitespaceByteCandidate(ch)))return look-start+i+1;
        }
        look+=chunk;
    }
    return look==start?0:look-start+1;
}
