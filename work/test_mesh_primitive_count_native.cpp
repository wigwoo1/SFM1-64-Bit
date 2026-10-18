#include <windows.h>
#include <iostream>
#include "native_d3d9_indexed_draw_candidate.h"
#include "mesh_primitive_count_native_bytes.h"
int main(){
#ifdef _M_IX86
 auto memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!memory)return 1;
 memcpy(memory,meshPrimitiveCountNativeBytes,sizeof(meshPrimitiveCountNativeBytes));
 auto table=uint32_t(reinterpret_cast<uintptr_t>(memory+92));memcpy(memory+15,&table,4);
 for(unsigned n=0;n<6;++n){auto target=uint32_t(reinterpret_cast<uintptr_t>(memory+meshPrimitiveCountTargets[n]));memcpy(memory+92+n*4,&target,4);}
 DWORD old;if(!VirtualProtect(memory,4096,PAGE_EXECUTE_READ,&old))return 2;FlushInstructionCache(GetCurrentProcess(),memory,4096);
 auto native=reinterpret_cast<uint32_t(__thiscall*)(void*,uint32_t,uint32_t)>(memory);
#endif
 uint32_t random=0x98312476;uint64_t digest=1469598103934665603ull;unsigned cases=0;
 for(unsigned i=0;i<65536;++i)for(uint32_t type=0;type<8;++type){
  random^=random<<13;random^=random>>17;random^=random<<5;uint32_t indices=random,vertices=random^0xabcdef97;
  if(i<6){uint32_t edges[]={0,1,2,0x7fffffff,0x80000000,0xffffffff};indices=edges[i];}
  auto packet=nativeD3d9IndexedPacketCandidate(type,-17,19,vertices,23,indices);
#ifdef _M_IX86
  uint32_t mesh[20]{};mesh[19]=type;
  if(native(mesh,vertices,indices)!=packet.primitiveCount)return 3;
#endif
  if(packet.primitiveType!=type||packet.baseVertex!=-17||packet.minVertex!=19||packet.vertexCount!=vertices||packet.firstIndex!=23)return 4;
  digest^=packet.primitiveCount;digest*=1099511628211ull;++cases;
 }
#ifdef _M_IX86
 VirtualFree(memory,0,MEM_RELEASE);
#endif
 std::cout<<"primitive_count_cases="<<cases<<" digest="<<digest<<" indexed_packet_integration=1\n";
}
