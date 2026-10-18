#pragma once
#include "graph_bookmark_insertion_candidate.h"
template<class Adapter>
void graphBookmarkGrowthCandidate(std::uint32_t increment,Adapter& a){
    const auto grow=a.growSize();
    if(graphInsertSigned(grow)<0)return;
    auto count=a.capacity();const auto requested=count+increment;
    if(grow){
        const auto quotient=graphInsertSigned(requested-1)/graphInsertSigned(grow);
        count=(static_cast<std::uint32_t>(quotient)+1)*grow;
    }else{
        if(!count)count=8;
        while(graphInsertSigned(count)<graphInsertSigned(requested))count+=count;
    }
    if(graphInsertSigned(count)<graphInsertSigned(requested)){
        if(count==0 && graphInsertSigned(requested)<=-1)count=0xffffffff;
        else while(graphInsertSigned(count)<graphInsertSigned(requested)){
            const auto sum=graphInsertSigned(count+requested);
            count=static_cast<std::uint32_t>(sum/2);
        }
    }
    auto* old=a.sourceData();a.setCapacity(count);
    a.setStorage(old?a.reallocate(old,count*4u):a.allocate(count*4u));
}
