#include <windows.h>
#include <fstream>
#include <array>
#include <cstring>
int main(){unsigned char bytes[]={
#include "native_local_scale_kernel.inc"
};auto code=VirtualAlloc(nullptr,sizeof(bytes),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!code)return 1;std::memcpy(code,bytes,sizeof(bytes));DWORD old;if(!VirtualProtect(code,sizeof(bytes),PAGE_EXECUTE_READ,&old))return 2;FlushInstructionCache(GetCurrentProcess(),code,sizeof(bytes));std::ofstream output("work/native-local-scale-oracle.bin",std::ios::binary);for(float scale:{0.9713f,0.0f,-1.0f,0.5f,1.0f,2.0f}){std::array<float,12>input{1.25f,-2.75f,3.125f,10.0f,-4.5f,5.75f,6.125f,20.0f,7.75f,8.125f,-9.5f,30.0f};auto result=input;float*matrix=result.data();
 __asm {push esi
 mov esi,matrix
 movss xmm0,scale
 call code
 pop esi}
 output.write(reinterpret_cast<char*>(&scale),4);output.write(reinterpret_cast<char*>(input.data()),48);output.write(reinterpret_cast<char*>(result.data()),48);}output.close();VirtualFree(code,0,MEM_RELEASE);return output?0:3;}
