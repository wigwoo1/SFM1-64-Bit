#pragma once
#include <cstdint>
inline std::uint32_t graphBookmarkUndoAllocationBytes(std::int32_t count){
 const auto product=std::uint64_t(std::uint32_t(count))*4;
 return product>0xffffffffu?0xffffffffu:std::uint32_t(product);
}
template<class Adapter>void graphBookmarkUndoSnapshotCandidate(const std::uint32_t* source,int count,Adapter& a){
 if(!source||count<=0){a.setBuffer(nullptr);a.setCount(0);return;}
 auto* buffer=a.allocate1025ea80(graphBookmarkUndoAllocationBytes(count));
 if(buffer)for(int i=0;i<count;++i)buffer[i]=0x80000000u;
 a.setBuffer(buffer);
 for(int i=0;i<count;++i){const auto value=source[i];a.liveBuffer()[i]=value;}
 a.setCount(count);
}
