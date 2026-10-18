#pragma once
#include <cstdint>
template<class Adapter>bool graphBookmarkEqualityCandidate(std::int32_t count,int component,Adapter& a){
 if(a.bookmarkCount105079b0(component)!=count)return false;
 for(std::int32_t i=0;i<count;++i){const auto source=a.sourceMappedTime(component,i);if(source!=a.bookmarkTime10517210(i,component))return false;}
 return true;
}
