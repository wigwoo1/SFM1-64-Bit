#pragma once
#include "buffer_read_check_candidate.h"
#include <cstring>
template<class Adapter>
void convertExternalBufferCandidate(std::uint32_t grow,Adapter& a){
    if(bufferSigned32Candidate(a.growSize())>=0)return;
    const auto capacity=a.capacity();
    a.setGrowSize(grow);
    if(capacity){
        auto* owned=a.allocate(capacity);
        std::memmove(owned,a.sourceData(),capacity);
        a.setStorage(owned);
    }else a.setStorage(nullptr);
}
