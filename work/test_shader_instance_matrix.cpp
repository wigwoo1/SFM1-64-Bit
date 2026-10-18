#include <windows.h>
#include <float.h>
#include <iostream>
#include "studio_shader_instance_state_candidate.h"
#include "shader_instance_matrix_native_bytes.h"
int main(){
#ifdef _M_IX86
 unsigned control;_controlfp_s(&control,_MCW_EM,_MCW_EM);
 auto memory=VirtualAlloc(nullptr,sizeof(shaderInstanceMatrixNativeBytes),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
 if(!memory)return 1;memcpy(memory,shaderInstanceMatrixNativeBytes,sizeof(shaderInstanceMatrixNativeBytes));DWORD old;
 if(!VirtualProtect(memory,sizeof(shaderInstanceMatrixNativeBytes),PAGE_EXECUTE_READ,&old))return 2;
 FlushInstructionCache(GetCurrentProcess(),memory,sizeof(shaderInstanceMatrixNativeBytes));
 auto native=reinterpret_cast<void(__stdcall*)(uint32_t*,const uint32_t*)>(memory);
#endif
 uint32_t random=0x98234567u;uint64_t digest=1469598103934665603ull;
 for(unsigned i=0;i<65536;++i){
  std::array<uint32_t,16> source;
  for(auto& v:source){random^=random<<13;random^=random>>17;random^=random<<5;v=random;}
  // Exercise signaling NaNs at every source position in both signs.
  if(i%4==0)source[(i/4)%16]=0x7f800001u|((i&32)?0x80000000u:0);
  auto result=studioShaderInstanceMatrixCandidate(source);
#ifdef _M_IX86
  std::array<uint32_t,16> actual;actual.fill(0xcccccccc);native(actual.data(),source.data());
  for(unsigned j=0;j<12;++j)if(actual[j]!=result[j])return 3;
#endif
  auto integerResult=studioShaderInstanceMatrixCandidate(source,true);
  for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)if(integerResult[r*4+c]!=source[c*4+r])return 4;
  StudioShaderInstanceStateCandidate state;auto instance=studioShaderInstanceStateFromMatrixCandidate(state,source);
  if(state.currentMatrix!=result||instance.instance.scalars[2]!=1)return 5;
  for(auto v:result){digest^=v;digest*=1099511628211ull;}
 }
#ifdef _M_IX86
 VirtualFree(memory,0,MEM_RELEASE);
#endif
 std::cout<<"matrix_cases=65536 digest="<<digest<<" integrated_instance_getter=1\n";
}
