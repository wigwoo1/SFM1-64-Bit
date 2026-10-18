#pragma once
#include "buffer_string_length_candidate.h"
#include "buffer_whitespace_candidate.h"
#include "buffer_byte_read_candidate.h"
#include <cstring>
template<class Adapter>
void extractBufferStringCandidate(std::uint8_t* output,std::uint32_t capacity,Adapter& a){
    if(a.errorByte()!=0){output[0]=0;return;}
    if(!capacity)capacity=0x7fffffff;
    const auto length=bufferStringLengthCandidate(a);
    if(a.flagsByte()&1)skipBufferWhitespaceCandidate(a);
    if(!length){output[0]=0;a.setErrorByte(static_cast<std::uint8_t>(a.errorByte()|2));return;}
    const bool truncated=bufferSigned32Candidate(length)>bufferSigned32Candidate(capacity);
    const auto count=(truncated?capacity:length)-1;
    if(bufferSigned32Candidate(count)>0 && checkBufferReadCandidate(count,a)){
        std::memmove(output,a.sourceData()+(a.getPosition()-a.bufferOffset()),count);
        a.setGetPosition(a.getPosition()+count);
    }
    output[count]=0;
    if(truncated){
        a.setGetPosition(a.getPosition()+length-capacity);
        const auto get=a.getPosition();
        if(bufferSigned32Candidate(get)>bufferSigned32Candidate(a.maxGet()))a.setErrorByte(static_cast<std::uint8_t>(a.errorByte()|2));
        else{
            const auto offset=a.bufferOffset();
            a.setErrorByte(static_cast<std::uint8_t>(a.errorByte()&~2u));
            if(bufferSigned32Candidate(get)<bufferSigned32Candidate(offset) ||
               bufferSigned32Candidate(get)>=bufferSigned32Candidate(offset+a.capacity()))a.readOverflow(0xffffffff);
        }
    }
    if(!(a.flagsByte()&1)){std::uint8_t ignored;readBufferByteCandidate(&ignored,a);}
}
