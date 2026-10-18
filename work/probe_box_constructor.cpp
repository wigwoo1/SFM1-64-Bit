#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#ifndef NATIVE_BOX_CONSTRUCTOR
#include "source_optimized_box_record_candidate.h"
#endif
#ifdef NATIVE_BOX_CONSTRUCTOR
#define NOMINMAX
#include <windows.h>
#include "native_box_constructor_bytes.h"
#endif
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("input output required");std::ifstream input(argv[1],std::ios::binary);std::ofstream output(argv[2],std::ios::binary);if(!input||!output)throw std::runtime_error("workspace files unavailable");
#ifdef NATIVE_BOX_CONSTRUCTOR
 auto base=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x420000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("base unavailable");std::memcpy(base+0x14a5b0,nativeConstructorBytes,sizeof(nativeConstructorBytes));float one=1,minus=-1;uint32_t sign=0x80000000;std::memcpy(base+0x31bdec,&one,4);std::memcpy(base+0x31d03c,&minus,4);std::memcpy(base+0x31cbb0,&sign,4);DWORD old;if(!VirtualProtect(base+0x140000,0x10000,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protection failed");FlushInstructionCache(GetCurrentProcess(),base,0x420000);using Function=void(__cdecl*)(void*,void*,void*,void*,void*);auto native=reinterpret_cast<Function>(base+0x14a5b0);
#endif
 std::array<unsigned char,144>query{};size_t count=0;
 while(input.read(reinterpret_cast<char*>(query.data()),query.size())){
  std::array<unsigned char,48>box;box.fill(0xa5);std::array<unsigned char,120>planes{};std::array<unsigned char,48>sides{};std::array<unsigned char,432>textures{};
  for(size_t i=0;i<6;++i){std::memcpy(planes.data()+20*i,query.data()+24*i,20);uint16_t plane=uint16_t(i);int16_t texture;std::memcpy(&texture,query.data()+24*i+20,2);if(texture>=0)texture=int16_t(i);std::memcpy(sides.data()+8*i,&plane,2);std::memcpy(sides.data()+8*i+2,&texture,2);sides[8*i+7]=query[24*i+22];uint32_t surface=query[24*i+23]*257u;std::memcpy(textures.data()+72*i+68,&surface,4);}
#ifdef NATIVE_BOX_CONSTRUCTOR
  std::array<uint32_t,2>brush{1,6};native(box.data(),brush.data(),sides.data(),planes.data(),textures.data());
#else
  std::vector<SourceBoxRecordPlaneCandidate> ownedPlanes(6);std::array<SourceBoxRecordSideCandidate,6> ownedSides{};std::vector<uint32_t> ownedSurfaces(6);
  for(size_t i=0;i<6;++i){auto&p=ownedPlanes[i];std::memcpy(p.normal.data(),planes.data()+20*i,12);std::memcpy(&p.distance,planes.data()+20*i+12,4);p.type=planes[20*i+16];int16_t texture;std::memcpy(&texture,sides.data()+8*i+2,2);ownedSides[i]={uint32_t(i),texture,sides[8*i+7]};std::memcpy(&ownedSurfaces[i],textures.data()+72*i+68,4);}
  sourceOptimizedBoxRecordCandidate(ownedPlanes,ownedSides,ownedSurfaces,box);
#endif
  output.write(reinterpret_cast<const char*>(box.data()),box.size());++count;
 }
 if(!input.eof()||input.gcount()!=0||!count)throw std::runtime_error("invalid input length");output.flush();if(!output)throw std::runtime_error("write failed");
#ifdef NATIVE_BOX_CONSTRUCTOR
 if(std::memcmp(base+0x14a5b0,nativeConstructorBytes,sizeof(nativeConstructorBytes)))throw std::runtime_error("body changed");VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
