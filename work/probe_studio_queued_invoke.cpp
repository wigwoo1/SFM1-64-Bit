#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "studio_decal_queued_call_candidate.h"
static uint32_t received[3];
#ifdef _WIN64
static void callback(uintptr_t owner,uintptr_t argument){received[0]=uint32_t(owner);received[1]=uint32_t(argument);++received[2];}
#else
#include "studio_queued_invoke_native_init.h"
static void __fastcall callback(void* owner,void*,uint32_t argument){received[0]=uint32_t(owner);received[1]=argument;++received[2];}
#endif
static void check(bool v){if(!v)throw std::runtime_error("queued invoke parity");}
int main(int argc,char**argv){try{check(argc==2);
#ifdef _WIN64
std::ifstream file(argv[1],std::ios::binary);
#else
std::ofstream file(argv[1],std::ios::binary);auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code);std::memcpy(code,studioQueuedInvokeNativeBytes,sizeof(studioQueuedInvokeNativeBytes));DWORD old;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old));FlushInstructionCache(GetCurrentProcess(),code,4096);using Invoke=void(__thiscall*)(void*);auto invoke=reinterpret_cast<Invoke>(code);
#endif
uint32_t state=0x98765432;for(unsigned i=0;i<65536;++i){state=state*1664525u+1013904223u;uint32_t owner=state;state=state*1664525u+1013904223u;uint32_t argument=state;std::memset(received,0,sizeof(received));
#ifdef _WIN64
StudioDecalQueuedCallCandidate call;call.owner=owner;call.argument=argument;call.method=&callback;call.invoke();check(call.owner==owner&&call.argument==argument&&call.references.load()==1);uint32_t expected[3];check(bool(file.read(reinterpret_cast<char*>(expected),sizeof(expected))));check(std::memcmp(received,expected,sizeof(received))==0);
#else
uint32_t object[9];std::memset(object,0xa5,sizeof(object));object[4]=uint32_t(&callback);object[5]=owner;object[6]=argument;uint32_t before[9];std::memcpy(before,object,sizeof(object));invoke(object);check(std::memcmp(object,before,sizeof(object))==0);file.write(reinterpret_cast<char*>(received),sizeof(received));
#endif
}
#ifdef _WIN64
check(file.peek()==std::char_traits<char>::eof());
#else
file.flush();check(bool(file));VirtualFree(code,0,MEM_RELEASE);
#endif
std::cout<<"queued_invoke_cases=65536\n";return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
