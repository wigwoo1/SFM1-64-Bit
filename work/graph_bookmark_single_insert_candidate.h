#pragma once
#include "graph_bookmark_insertion_candidate.h"
template<class Adapter>int graphBookmarkSingleInsertCandidate(int index,const std::uint32_t* value,Adapter& a){
 const auto old=a.count0c(),capacity=a.capacity4();
 if(graphInsertSigned(std::uint32_t(old)+1)>capacity)a.grow102bdbd0(graphInsertSigned(std::uint32_t(old)-std::uint32_t(capacity)+1));
 a.setCount0c(graphInsertSigned(std::uint32_t(a.count0c())+1));
 auto* base=a.base0();a.setCachedBase10(base);
 const auto trailing=graphInsertSigned(std::uint32_t(a.count0c())-std::uint32_t(index)-1);
 if(trailing>0)a.shift10974fe0(base,index,1,std::uint32_t(trailing)*4u);
 if(auto* slot=a.insertedSlot(index,0))*slot=*value;
 return index;
}
