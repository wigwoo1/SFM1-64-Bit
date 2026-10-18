#include "source_box_classification_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#ifdef NATIVE_BOX_CLASSIFICATION
#define NOMINMAX
#include <windows.h>
#include "native_box_classification_bytes.h"
// Establish the recovered loader's register/local frame around its unaltered
// classification block. A test-only RET follows the block's exit boundary.
__declspec(naked) static void invoke(void*,void*){
 __asm {push ebp
 mov ebp,esp
 sub esp,140h
 push ebx
 push esi
 push edi
 mov ebx,dword ptr [ebp+8]
 call dword ptr [ebp+0ch]
 pop edi
 pop esi
 pop ebx
 mov esp,ebp
 pop ebp
 ret}
}
#endif
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("input output required");std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);if(!in||!out)throw std::runtime_error("workspace files unavailable");
#ifdef NATIVE_BOX_CLASSIFICATION
 auto base=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x160000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("base unavailable");std::memcpy(base+0x14b5a5,nativeClassificationBytes,sizeof(nativeClassificationBytes));base[0x14b668]=0xc3;DWORD old;if(!VirtualProtect(base+0x140000,0x10000,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protection failed");FlushInstructionCache(GetCurrentProcess(),base,0x160000);
#endif
 std::array<uint8_t,8>query{};size_t cases=0;while(in.read(reinterpret_cast<char*>(query.data()),8)){uint16_t count;std::memcpy(&count,query.data(),2);std::array<uint8_t,6>types{};std::memcpy(types.data(),query.data()+2,6);uint16_t result=count;
#ifdef NATIVE_BOX_CLASSIFICATION
  std::array<uint8_t,0xb0>world{};std::array<uint8_t,120>planes{};std::array<uint8_t,48>sides{};std::array<uint32_t,2>brush{1,uint32_t(count)};
  for(uint16_t i=0;i<6;++i){planes[20*i+16]=types[i];std::memcpy(sides.data()+8*i,&i,2);}uint32_t planePtr=reinterpret_cast<uint32_t>(planes.data()),brushPtr=reinterpret_cast<uint32_t>(brush.data()),sidePtr=reinterpret_cast<uint32_t>(sides.data()),one=1;std::memcpy(world.data()+0x78,&planePtr,4);std::memcpy(world.data()+0xa8,&brushPtr,4);std::memcpy(world.data()+0xa4,&one,4);
  // Raw-side pointer is a loader local at EBP-138h. Pass it through a tiny
  // entry shim before jumping into the original block.
  unsigned char shim[]={0xc7,0x85,0xc8,0xfe,0xff,0xff,0,0,0,0,0xe9,0,0,0,0};std::memcpy(shim+6,&sidePtr,4);int32_t relative=0x14b5a5-(0x130000+15);std::memcpy(shim+11,&relative,4);std::memcpy(base+0x130000,shim,sizeof(shim));DWORD protection;if(!VirtualProtect(base+0x130000,0x1000,PAGE_EXECUTE_READWRITE,&protection))throw std::runtime_error("shim protection failed");FlushInstructionCache(GetCurrentProcess(),base+0x130000,sizeof(shim));invoke(world.data(),base+0x130000);result=uint16_t(brush[1]);
#else
  if(sourceBoxClassificationCandidate(count,types))result=65535;
#endif
  out.write(reinterpret_cast<const char*>(&result),2);++cases;
 }if(!in.eof()||in.gcount()!=0||!cases)throw std::runtime_error("invalid input length");out.flush();if(!out)throw std::runtime_error("output failed");
#ifdef NATIVE_BOX_CLASSIFICATION
 if(std::memcmp(base+0x14b5a5,nativeClassificationBytes,sizeof(nativeClassificationBytes)))throw std::runtime_error("original block changed");VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
