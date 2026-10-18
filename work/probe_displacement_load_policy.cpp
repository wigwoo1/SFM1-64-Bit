#include "source_displacement_load_policy_candidate.h"
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#ifdef NATIVE_LOAD_POLICY
#define NOMINMAX
#include <windows.h>
#include "native_displacement_load_policy_bytes.h"
static int32_t hdrMode=0,hdrLength=0;static uint32_t sizeCalls=0,readCount=0;static std::array<std::array<uint32_t,2>,2>reads{};
static int __fastcall getHdr(void*,void*){return hdrMode;}
static int __cdecl getSize(int id){if(id!=58)ExitProcess(91);++sizeCalls;return hdrLength;}
static void __fastcall readFixture(void*reader,void*,uint32_t offset,uint32_t size,void*dest){uint32_t kind;std::memcpy(&kind,reader,4);if(kind!=readCount||readCount>=2||size>5780)ExitProcess(92);reads[readCount++]={offset,size};std::memset(dest,0,size);}
__declspec(naked) static uint32_t choose(void*){__asm{
 push ebp
 mov ebp,esp
 sub esp,1000h
 mov dword ptr [esp],0
 sub esp,1000h
 mov dword ptr [esp],0
 sub esp,900h
 mov dword ptr [esp],0
 push ebx
 push esi
 push edi
 call dword ptr [ebp+8]
 pop edi
 pop esi
 pop ebx
 add esp,2900h
 pop ebp
 ret
}}
__declspec(naked) static void plan(void*,uint32_t,uint32_t,uint32_t,void*){__asm{
 push ebp
 mov ebp,esp
 sub esp,1000h
 mov dword ptr [esp],0
 sub esp,1000h
 mov dword ptr [esp],0
 sub esp,900h
 mov dword ptr [esp],0
 push ebx
 push esi
 push edi
 mov eax,dword ptr [ebp+0ch]
 mov dword ptr [ebp-1b70h],eax
 mov eax,dword ptr [ebp+10h]
 mov dword ptr [ebp-27ech],eax
 mov eax,dword ptr [ebp+14h]
 mov dword ptr [ebp-27f0h],eax
 mov dword ptr [ebp-26b0h],0
 mov dword ptr [ebp-27d0h],1
 call dword ptr [ebp+8]
 mov ecx,dword ptr [ebp+18h]
 mov eax,dword ptr [ebp-27ech]
 mov dword ptr [ecx],eax
 mov eax,dword ptr [ebp-27f0h]
 mov dword ptr [ecx+4],eax
 pop edi
 pop esi
 pop ebx
 add esp,2900h
 pop ebp
 ret
}}
#endif
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("input output required");std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);if(!in||!out)throw std::runtime_error("workspace files");
#ifdef NATIVE_LOAD_POLICY
 auto base=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x6a0000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("native base unavailable");for(const auto&f:loadPolicyFragments){std::memcpy(base+f.offset,f.bytes,f.size);base[f.offset+f.size]=0xc3;}
 auto jump=[&](size_t at,uintptr_t target){base[at]=0xe9;int32_t d=int32_t(target-(reinterpret_cast<uintptr_t>(base)+at+5));std::memcpy(base+at+1,&d,4);};jump(0xe1e20,reinterpret_cast<uintptr_t>(&getSize));jump(0xe0370,reinterpret_cast<uintptr_t>(&readFixture));
 std::array<uintptr_t,35>vtable{};vtable[34]=reinterpret_cast<uintptr_t>(&getHdr);auto vp=vtable.data();auto iface=&vp;uint32_t interfaceAddress=reinterpret_cast<uint32_t>(iface);std::memcpy(base+0x694194,&interfaceAddress,4);
 DWORD old;for(size_t at:{size_t(0xe0000),size_t(0x140000)})if(!VirtualProtect(base+at,0x10000,PAGE_EXECUTE_READ,&old))throw std::runtime_error("code protection");FlushInstructionCache(GetCurrentProcess(),base,0x6a0000);
#endif
 std::array<uint32_t,5>q{};size_t cases=0;while(in.read(reinterpret_cast<char*>(q.data()),20)){
  std::array<uint32_t,8>result{};int32_t hdr,bytes;std::memcpy(&hdr,&q[0],4);std::memcpy(&bytes,&q[1],4);if(q[2]<1||q[2]>4)throw std::runtime_error("fixture power");
#ifdef NATIVE_LOAD_POLICY
  hdrMode=hdr;hdrLength=bytes;sizeCalls=readCount=0;result[0]=choose(base+0x14c0f5);result[1]=sizeCalls;std::array<uint32_t,2>next{};plan(base+0x14c2d6,q[2],q[3],q[4],next.data());if(readCount!=2)throw std::runtime_error("read calls");result[2]=reads[0][0];result[3]=reads[0][1];result[4]=reads[1][0];result[5]=reads[1][1];result[6]=next[0];result[7]=next[1];
#else
  result[0]=sourceDisplacementFaceLumpCandidate(hdr,bytes);result[1]=hdr!=0;const auto p=sourceDisplacementReadPlanCandidate(q[2],q[3],q[4]);result[2]=p.vertexOffset;result[3]=p.vertexBytes;result[4]=p.triangleOffset;result[5]=p.triangleBytes;result[6]=p.nextVertex;result[7]=p.nextTriangle;
#endif
  out.write(reinterpret_cast<const char*>(result.data()),32);++cases;
 }if(!in.eof()||in.gcount()!=0||!cases)throw std::runtime_error("input length");out.flush();if(!out)throw std::runtime_error("output failure");
#ifdef NATIVE_LOAD_POLICY
 for(const auto&f:loadPolicyFragments)if(std::memcmp(base+f.offset,f.bytes,f.size))throw std::runtime_error("original block changed");VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS displacement loading policy cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
