#pragma once
#include <cstdint>
#include <cstring>
inline std::int32_t graphAliasDifference(std::int32_t a,std::int32_t b){auto bits=std::uint32_t(a)-std::uint32_t(b);std::int32_t result;std::memcpy(&result,&bits,4);return result;}
template<class Source,class Adapter>bool graphBookmarkAliasAssignmentCandidate(Source* source,std::int32_t count,Adapter& a){
 if(source!=a.destinationBase())return false;
 const auto old=a.destinationCount();
 if(old>count)a.remove103883e0(count,graphAliasDifference(old,count));
 else if(old<count)a.insert103771c0(old,graphAliasDifference(count,old));
 return true;
}
