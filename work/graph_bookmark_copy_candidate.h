#pragma once
#include <cstdint>
template<class Adapter>void graphBookmarkCopyCandidate(const std::uint32_t* source,int count,Adapter& a){
 auto* storage=a.vector8();a.setCount0c(storage,0);a.insert1017ee80(storage,0,count);
 for(int i=0;i<count;++i){const auto value=source[i];a.liveBase0(storage)[i]=value;}
}
