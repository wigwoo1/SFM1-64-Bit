#pragma once
#include <cstdint>
#include <cstring>
#include <algorithm>
// Index branch of100219e0. Positive count, valid disjoint buffers required.
// Source/destination support unaligned word access via byte copies.
template<class Copy>void studioMaterialIndexUploadCandidate(uintptr_t destination,uintptr_t source,int count,uint32_t vertexOffset,Copy copy){
 if(!vertexOffset){copy(destination,source,std::size_t(count)*2);return;}
 auto read=[&](int index){uint16_t value;std::memcpy(&value,reinterpret_cast<void*>(source+uintptr_t(index)*2),2);return uint16_t(value+uint16_t(vertexOffset));};
 int consumed=0;
 if((destination&3)==2){uint16_t value=read(0);std::memcpy(reinterpret_cast<void*>(destination),&value,2);consumed=1;}
 while(consumed<count){int chunk=std::min(16,count-consumed);uint16_t scratch[16];for(int i=0;i<chunk;++i)scratch[i]=read(consumed+i);copy(destination+uintptr_t(consumed)*2,reinterpret_cast<uintptr_t>(scratch),std::size_t(chunk)*2);consumed+=chunk;}
}
