#include "source_bsp_world_collision_data_candidate.h"
#include <fstream>
#include <iterator>
#include <iostream>
#include <algorithm>
#ifdef NATIVE_PLANE_IMPORT
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
// Simulate the two pending allocator arguments that the original block drops.
// No original instruction is patched: relocated internal branches end at an
// appended RET. This executes the import loop, not the full BSP loader.
__declspec(naked) static void invokeImport(void*,void*,void*,void*,uint32_t){__asm{
 push ebp
 mov ebp,esp
 push ebx
 push esi
 push edi
 sub esp,150h
 mov eax,dword ptr [ebp+0ch]
 mov ebx,dword ptr [ebp+10h]
 mov esi,dword ptr [ebp+14h]
 mov edi,dword ptr [ebp+18h]
 mov dword ptr [ebp-134h],ebx
 mov dword ptr [ebp-130h],edi
 lea edx,landed
 push edx
 sub esp,8
 jmp dword ptr [ebp+8]
landed:
 lea esp,[ebp-0ch]
 pop edi
 pop esi
 pop ebx
 pop ebp
 ret
}}
#endif
int main(int argc,char**argv){try{
 if(argc!=4)throw std::runtime_error("input output engine required");std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);if(!in||!out)throw std::runtime_error("workspace files");
#ifdef NATIVE_PLANE_IMPORT
 NativePrivateWorldImageFixture image(argv[3]);constexpr size_t bytes=0x1014b40b-0x1014b1f3;auto code=static_cast<uint8_t*>(VirtualAlloc(nullptr,bytes+1,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)throw std::runtime_error("fragment allocation");std::memcpy(code,image.base+0x14b1f3,bytes);code[bytes]=0xc3;const std::vector<uint8_t>snapshot(code,code+bytes+1);DWORD old;if(!VirtualProtect(code,bytes+1,PAGE_EXECUTE_READ,&old))throw std::runtime_error("fragment protection");FlushInstructionCache(GetCurrentProcess(),code,bytes+1);image.seal();
#endif
 uint32_t count=0;size_t batches=0,planes=0;
 while(in.read(reinterpret_cast<char*>(&count),4)){
  if(count>65536)throw std::runtime_error("batch limit");std::vector<uint8_t>source(size_t(count)*20+32,0x5a);if(!in.read(reinterpret_cast<char*>(source.data()+16),size_t(count)*20))throw std::runtime_error("batch truncated");const auto original=source;std::vector<uint8_t>result(size_t(count)*20);
#ifdef NATIVE_PLANE_IMPORT
  for(uint8_t fill:{uint8_t(0),uint8_t(0xa7)}){
   std::vector<uint8_t>dest(size_t(count)*20+32,0xe5),world(0xa0,0xd6);std::fill(dest.begin()+16,dest.end()-16,fill);const auto before=world;invokeImport(code,dest.data()+16,world.data(),source.data()+16,count);uint32_t n,p;std::memcpy(&n,world.data()+0x74,4);std::memcpy(&p,world.data()+0x78,4);if(n!=count||p!=reinterpret_cast<uint32_t>(dest.data()+16))throw std::runtime_error("native world fields");
   for(size_t i=0;i<world.size();++i)if((i<0x74||i>=0x7c)&&world[i]!=before[i])throw std::runtime_error("world guard");for(size_t i=0;i<16;++i)if(dest[i]!=0xe5||dest[dest.size()-1-i]!=0xe5)throw std::runtime_error("destination guard");for(size_t i=0;i<count;++i)if(dest[16+i*20+18]!=fill||dest[16+i*20+19]!=fill)throw std::runtime_error("reserved bytes overwritten");if(fill==0)std::copy(dest.begin()+16,dest.end()-16,result.begin());
  }
#else
  (void)argv[3];for(size_t i=0;i<count;++i){const auto*p=source.data()+16+20*i;std::array<float,3>normal;float distance;uint32_t type;std::memcpy(normal.data(),p,12);std::memcpy(&distance,p+12,4);std::memcpy(&type,p+16,4);const auto plane=sourceBspCollisionPlaneCandidate(normal,distance,type);auto*d=result.data()+20*i;std::memcpy(d,plane.normal.data(),12);std::memcpy(d+12,&plane.distance,4);const auto tags=plane.tags();std::memcpy(d+16,tags.data(),4);}
#endif
  if(source!=original)throw std::runtime_error("source changed");out.write(reinterpret_cast<const char*>(result.data()),result.size());++batches;planes+=count;
 }
 if(!in.eof()||in.gcount()!=0||!batches)throw std::runtime_error("input span");out.flush();if(!out)throw std::runtime_error("output write");
#ifdef NATIVE_PLANE_IMPORT
 if(std::memcmp(code,snapshot.data(),snapshot.size()))throw std::runtime_error("fragment changed");image.verify();VirtualFree(code,0,MEM_RELEASE);VirtualFree(image.base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS plane import batches="<<batches<<" planes="<<planes<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
