#pragma once
#include "graph_bookmark_alias_assignment_candidate.h"
inline int graphInsertSigned(std::uint32_t bits){int n;std::memcpy(&n,&bits,4);return n;}
template<class Adapter>int graphBookmarkInsertionCandidate(int start,int amount,Adapter& a){
 if(!amount)return start;
 const auto old=a.count0c(),capacity=a.capacity4();
 if(graphInsertSigned(std::uint32_t(old)+std::uint32_t(amount))>capacity)a.grow102bdbd0(graphInsertSigned(std::uint32_t(old)-std::uint32_t(capacity)+std::uint32_t(amount)));
 a.setCount0c(graphInsertSigned(std::uint32_t(a.count0c())+std::uint32_t(amount)));
 auto* base=a.base0();a.setCachedBase10(base);
 const auto trailing=graphInsertSigned(std::uint32_t(a.count0c())-std::uint32_t(start)-std::uint32_t(amount));
 if(trailing>0 && amount>0)a.shift10974fe0(base,start,amount,std::uint32_t(trailing)*4u);
 if(amount>0)for(int i=0;i<amount;++i)if(auto* slot=a.insertedSlot(start,i))*slot=0x80000000u;
 return start;
}
