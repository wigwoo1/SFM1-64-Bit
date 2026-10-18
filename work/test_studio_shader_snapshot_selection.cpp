#include <iostream>
#include <vector>
#include <windows.h>
#include <algorithm>
#include "shader_snapshot_selection_native_bytes.h"
#include "studio_shader_snapshot_selection_candidate.h"
struct Ops {
 uint32_t word,a,b,c;int32_t count;uintptr_t api;std::vector<unsigned> calls;
 uint32_t flags(){calls.push_back(0);return word;}
 uint32_t query17(uintptr_t p){if(p!=api)throw 1;calls.push_back(17);return a;}
 uint32_t query19(uintptr_t p){if(p!=api)throw 2;calls.push_back(19);return b;}
 int32_t query63(uintptr_t p){if(p!=api)throw 3;calls.push_back(63);return count;}
 uint32_t query29(uintptr_t p,uint32_t n){if(p!=api||n)throw 4;calls.push_back(29);return c;}
};
#ifdef _M_IX86
static Ops* nativeOps;
static uint32_t __fastcall query17(void* p,void*){return nativeOps->query17(reinterpret_cast<uintptr_t>(p));}
static uint32_t __fastcall query19(void* p,void*){return nativeOps->query19(reinterpret_cast<uintptr_t>(p));}
static int32_t __fastcall query63(void* p,void*){return nativeOps->query63(reinterpret_cast<uintptr_t>(p));}
static uint32_t __fastcall query29(void* p,void*,uint32_t n){return nativeOps->query29(reinterpret_cast<uintptr_t>(p),n);}
#endif
int main(){unsigned cases=0;
#ifdef _M_IX86
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,8192,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(!code)return 2;
 memcpy(code,shaderSnapshotSelectionNativeBytes,sizeof(shaderSnapshotSelectionNativeBytes));
 for(auto& patch:shaderSnapshotSelectionPatches){auto pointer=uint32_t(reinterpret_cast<uintptr_t>(code+patch[1]));memcpy(code+patch[0],&pointer,4);}
 DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 3;FlushInstructionCache(GetCurrentProcess(),code,4096);
 auto native=reinterpret_cast<uint32_t(__stdcall*)(void*,void*)>(code);
 uintptr_t table[64]{};table[17]=reinterpret_cast<uintptr_t>(query17);table[19]=reinterpret_cast<uintptr_t>(query19);table[63]=reinterpret_cast<uintptr_t>(query63);table[29]=reinterpret_cast<uintptr_t>(query29);
 uintptr_t apiObject=reinterpret_cast<uintptr_t>(table);
#endif
 uint64_t digest=1469598103934665603ull;
 for(unsigned combo=0;combo<128;++combo)for(bool shadow:{false,true})for(uint32_t a:{0u,1u,0x100u,0x101u})for(uint32_t b:{0u,1u,0x100u,0x101u})for(int32_t count:{-1,0,1,2})for(uint32_t bits=0;bits<4;++bits){
  uint32_t flags=0;const unsigned positions[]={10,13,9,11,20,21,31};for(unsigned i=0;i<7;++i)if(combo&(1u<<i))flags|=1u<<positions[i];
  uintptr_t api=0;
#ifdef _M_IX86
  api=reinterpret_cast<uintptr_t>(&apiObject);
#else
  api=uintptr_t(0x123456789abcull);
#endif
  Ops ops{flags,a,b,bits,count,api,{}};StudioShaderSnapshotSelectionStateCandidate state{shadow?api+64:0,api+128};
  auto result=studioShaderSnapshotSelectionCandidate(state,api,ops);
  uint32_t expected=0;
  if(shadow?(flags&1024)!=0:uint8_t(a)!=0)expected|=2;
  if(shadow?(flags&8192)!=0:uint8_t(b)!=0)expected|=16;
  if(count>0)expected|=4;if((flags&512)&&(flags&2048))expected|=8;
  if(shadow?(flags&1048576)!=0:(bits&1)!=0)expected|=32;
  if(shadow?(flags&2097152)!=0:(bits&2)!=0)expected|=64;
  auto expectedCalls=shadow?std::vector<unsigned>{0,0,63,0}:std::vector<unsigned>{17,19,63,0,29};
  if(result!=expected||state.currentApi||ops.calls!=expectedCalls)return 1;
#ifdef _M_IX86
  Ops original{flags,a,b,bits,count,api,{}};nativeOps=&original;
  uint32_t parameter[3]={0xcccccccc,0xdddddddd,flags};uintptr_t parameters[3]={0,0,reinterpret_cast<uintptr_t>(parameter)};
  *reinterpret_cast<uint32_t*>(code+4096)=shadow?1:0;*reinterpret_cast<uint32_t*>(code+4100)=0xa5a5a5a5;
  auto nativeResult=native(parameters,&apiObject);
  ops.calls.erase(std::remove(ops.calls.begin(),ops.calls.end(),0u),ops.calls.end());
  if(nativeResult!=result||*reinterpret_cast<uint32_t*>(code+4100)||*reinterpret_cast<uint32_t*>(code+4096)!=unsigned(shadow)||original.calls!=ops.calls)return 4;
#endif
  digest^=result;digest*=1099511628211ull;++cases;
 }
#ifdef _M_IX86
 VirtualFree(code,0,MEM_RELEASE);
#endif
 std::cout<<"snapshot_selection_cases="<<cases<<" digest="<<digest<<" low_byte_queries=1 shadow_runtime_paths=1 callback_order=1\n";
}
