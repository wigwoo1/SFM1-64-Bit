#pragma once
#include "buffer_read_check_candidate.h"
template<class Adapter>
void growBufferStorageCandidate(std::uint32_t increment,Adapter& a){
    const auto grow=a.growSize();
    if(bufferSigned32Candidate(grow)<0)return;
    auto count=a.capacity();const auto requested=count+increment;
    if(grow){
        const auto quotient=bufferSigned32Candidate(requested-1)/bufferSigned32Candidate(grow);
        count=(static_cast<std::uint32_t>(quotient)+1)*grow;
    }else{
        if(!count)count=32;
        while(bufferSigned32Candidate(count)<bufferSigned32Candidate(requested))count+=count;
    }
    if(bufferSigned32Candidate(count)<bufferSigned32Candidate(requested)){
        if(count==0 && bufferSigned32Candidate(requested)<=-1)count=0xffffffff;
        else while(bufferSigned32Candidate(count)<bufferSigned32Candidate(requested)){
            const auto sum=bufferSigned32Candidate(count+requested);
            count=static_cast<std::uint32_t>(sum/2);
        }
    }
    auto* old=a.sourceData();a.setCapacity(count);
    a.setStorage(old?a.reallocate(old,count):a.allocate(count));
}
