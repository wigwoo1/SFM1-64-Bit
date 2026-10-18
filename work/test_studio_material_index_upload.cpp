#include <vector>
#include <iostream>
#include "studio_material_index_upload_candidate.h"
int main(){unsigned cases=0;
for(int count=1;count<=257;++count)for(unsigned alignment: {0u,2u})for(uint32_t offset: {0u,1u,32767u,65535u,65536u}){
std::vector<unsigned char> source(size_t(count)*2+64,0xa5),destination(size_t(count)*2+64,0xa5);auto src=reinterpret_cast<uintptr_t>(source.data()+16);auto dst=reinterpret_cast<uintptr_t>(destination.data()+16+alignment);for(int i=0;i<count;++i){uint16_t value=uint16_t(i*977+65500);std::memcpy(reinterpret_cast<void*>(src+uintptr_t(i)*2),&value,2);}auto original=source;unsigned copies=0;
studioMaterialIndexUploadCandidate(dst,src,count,offset,[&](uintptr_t to,uintptr_t from,size_t bytes){++copies;std::memcpy(reinterpret_cast<void*>(to),reinterpret_cast<void*>(from),bytes);});
if(source!=original)return 1;for(int i=0;i<count;++i){uint16_t value;std::memcpy(&value,reinterpret_cast<void*>(dst+uintptr_t(i)*2),2);if(value!=uint16_t(uint16_t(i*977+65500)+uint16_t(offset)))return 1;}
for(size_t i=0;i<16+alignment;++i)if(destination[i]!=0xa5)return 1;for(size_t i=16+alignment+size_t(count)*2;i<destination.size();++i)if(destination[i]!=0xa5)return 1;
unsigned expected=offset==0?1u:unsigned((count-int(alignment==2)+15)/16);if(copies!=expected)return 1;++cases;
}std::cout<<"index_upload_cases="<<cases<<" wraparound_alignment_chunk_boundaries=1 source_and_guards_retained=1\n";}
