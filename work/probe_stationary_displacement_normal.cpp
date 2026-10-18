#include "source_stationary_displacement_normal_candidate.h"
#include <fstream>
#include <iostream>
#ifdef NATIVE_STATIONARY_NORMAL
#define NOMINMAX
#include <windows.h>
#include "native_stationary_displacement_normal_bytes.h"
#endif
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("input output required");std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);if(!in||!out)throw std::runtime_error("workspace files");
#ifdef NATIVE_STATIONARY_NORMAL
 auto base=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x6a0000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("native base unavailable");for(const auto&f:stationaryNormalFragments)std::memcpy(base+f.offset,f.bytes,f.size);DWORD old;if(!VirtualProtect(base+0x140000,0x20000,PAGE_EXECUTE_READ,&old))throw std::runtime_error("native protection");FlushInstructionCache(GetCurrentProcess(),base,0x6a0000);using Function=void(__cdecl*)(void*,const uint16_t*,int,float*,uint32_t,uint32_t*);auto native=reinterpret_cast<Function>(base+0x14cd40);
#endif
 std::array<uint32_t,57>q{};size_t count=0;while(in.read(reinterpret_cast<char*>(q.data()),sizeof(q))){
  if(q[4]>8)throw std::runtime_error("fixture reference count");std::array<float,3>point{},normal{};std::memcpy(point.data(),q.data(),12);std::memcpy(normal.data(),q.data()+13,12);uint32_t contents=q[16];std::vector<uint32_t>refs;for(size_t i=0;i<q[4];++i){if(q[5+i]>3)throw std::runtime_error("fixture reference");refs.push_back(q[5+i]);}
#ifdef NATIVE_STATIONARY_NORMAL
  std::array<std::array<uint8_t,244>,4>meshes{};std::array<uint16_t,8>nativeRefs{};for(size_t i=0;i<refs.size();++i)nativeRefs[i]=uint16_t(refs[i]);for(size_t i=0;i<4;++i){const auto*p=q.data()+17+10*i;std::memcpy(meshes[i].data()+4,p,12);std::memcpy(meshes[i].data()+0x14,p+3,12);std::memcpy(meshes[i].data()+0x60,p+6,12);std::memcpy(meshes[i].data()+0x20,p+9,4);}std::array<uint8_t,0x220>work{};std::memcpy(work.data(),point.data(),12);work[0x100]=1;auto pointer=reinterpret_cast<uint32_t>(meshes.data());std::memcpy(base+0x637820,&pointer,4);native(work.data(),nativeRefs.data(),int(refs.size()),normal.data(),q[3],&contents);
#else
  std::vector<SourceDisplacementRuntimeMeshCandidate>meshes(4);for(size_t i=0;i<4;++i){auto*p=q.data()+17+10*i;std::memcpy(meshes[i].tree.expandedBounds.data(),p,24);std::memcpy(meshes[i].baseNormal.data(),p+6,12);meshes[i].contents=p[9];}sourceStationaryDisplacementNormalCandidate(meshes,refs,point,q[3],normal,contents);
  // Invalid reference and invalid bounds must preserve caller outputs.
  auto before=normal;auto beforeContents=contents;bool rejected=false;try{sourceStationaryDisplacementNormalCandidate(meshes,{0,4},point,q[3],normal,contents);}catch(const std::out_of_range&){rejected=true;}if(!rejected||normal!=before||contents!=beforeContents)throw std::runtime_error("reference failure changed outputs");meshes[0].tree.expandedBounds[0]=std::numeric_limits<float>::infinity();rejected=false;try{sourceStationaryDisplacementNormalCandidate(meshes,{0},point,q[3],normal,contents);}catch(const std::runtime_error&){rejected=true;}if(!rejected||normal!=before||contents!=beforeContents)throw std::runtime_error("bounds failure changed outputs");
#endif
  out.write(reinterpret_cast<const char*>(normal.data()),12);out.write(reinterpret_cast<const char*>(&contents),4);++count;
 }if(!in.eof()||in.gcount()!=0||!count)throw std::runtime_error("input length");out.flush();if(!out)throw std::runtime_error("output failure");
#ifdef NATIVE_STATIONARY_NORMAL
 for(const auto&f:stationaryNormalFragments)if(std::memcmp(base+f.offset,f.bytes,f.size))throw std::runtime_error("original code changed");VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS stationary displacement normal cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
