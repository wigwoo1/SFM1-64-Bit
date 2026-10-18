#include "source_displacement_surface_orientation_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#ifdef NATIVE_SURFACE_ORIENTATION
#define NOMINMAX
#include <windows.h>
#include "native_displacement_surface_orientation_bytes.h"
#endif
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("input output required");std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);if(!in||!out)throw std::runtime_error("workspace files");
#ifdef NATIVE_SURFACE_ORIENTATION
 auto base=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x420000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("native base unavailable");for(const auto&f:surfaceOrientationFragments)std::memcpy(base+f.offset,f.bytes,f.size);const float limit=1000000000.f;std::memcpy(base+0x31cd30,&limit,4);
 DWORD old;for(size_t at:{size_t(0x130000),size_t(0xb0000),size_t(0),size_t(0x2d0000)})if(!VirtualProtect(base+at,0x10000,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protection failed");FlushInstructionCache(GetCurrentProcess(),base,0x420000);
 using Select=int32_t(__thiscall*)(void*);using Rotate=void(__thiscall*)(void*);auto select=reinterpret_cast<Select>(base+0x13cfa0);auto rotate=reinterpret_cast<Rotate>(base+0x13d0c0);
#endif
 std::array<uint8_t,0x2e8>raw{};size_t count=0;
 while(in.read(reinterpret_cast<char*>(raw.data()),raw.size())){
  int32_t selected=-1;
#ifdef NATIVE_SURFACE_ORIENTATION
  selected=select(raw.data());if(selected>=0&&selected<=3)rotate(raw.data());
#else
  SourceDisplacementSurfaceOrientationCandidate s;
  std::memcpy(s.corners.data(),raw.data()+8,48);std::memcpy(s.secondaryVectors.data(),raw.data()+0x38,48);std::memcpy(s.pairs.data(),raw.data()+0x68,32);std::memcpy(s.unrotatedScalars.data(),raw.data()+0x108,16);std::memcpy(s.attributes0.data(),raw.data()+0x118,64);std::memcpy(s.attributes1.data(),raw.data()+0x158,64);std::memcpy(s.vectors.data(),raw.data()+0x198,192);std::memcpy(&s.startCorner,raw.data()+0x2d8,4);std::memcpy(s.start.data(),raw.data()+0x2dc,12);
  selected=sourceDisplacementStartCornerCandidate(s);s.startCorner=selected;if(selected>=0)sourceDisplacementRotateSurfaceCandidate(s);
  std::memcpy(raw.data()+8,s.corners.data(),48);std::memcpy(raw.data()+0x38,s.secondaryVectors.data(),48);std::memcpy(raw.data()+0x68,s.pairs.data(),32);std::memcpy(raw.data()+0x108,s.unrotatedScalars.data(),16);std::memcpy(raw.data()+0x118,s.attributes0.data(),64);std::memcpy(raw.data()+0x158,s.attributes1.data(),64);std::memcpy(raw.data()+0x198,s.vectors.data(),192);std::memcpy(raw.data()+0x2d8,&s.startCorner,4);
#endif
  out.write(reinterpret_cast<const char*>(&selected),4);out.write(reinterpret_cast<const char*>(raw.data()),raw.size());++count;
 }
 if(!in.eof()||in.gcount()!=0||!count)throw std::runtime_error("input length");out.flush();if(!out)throw std::runtime_error("output failure");
#ifdef NATIVE_SURFACE_ORIENTATION
 for(const auto&f:surfaceOrientationFragments)if(std::memcmp(base+f.offset,f.bytes,f.size))throw std::runtime_error("original code changed");VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS surface orientation cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
