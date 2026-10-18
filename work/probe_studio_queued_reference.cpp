#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "studio_decal_queued_call_candidate.h"
static unsigned frees;
static void check(bool v){if(!v)throw std::runtime_error("queued reference parity");}
#ifndef _WIN64
#include "studio_queued_reference_native_init.h"
static void* expectedObject;
static void __cdecl freeObject(void* object){check(object==expectedObject);++frees;}
#endif
int main(int argc,char**argv){try{check(argc==2);
#ifdef _WIN64
std::ifstream file(argv[1],std::ios::binary);
#else
std::ofstream file(argv[1],std::ios::binary);auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code);initializeStudioQueuedReferenceNative(code,uint32_t(&freeObject));DWORD old;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old));FlushInstructionCache(GetCurrentProcess(),code,4096);using Reference=uint32_t(__thiscall*)(void*);auto add=reinterpret_cast<Reference>(code);auto release=reinterpret_cast<Reference>(code+32);uint32_t secondary[2]={uint32_t(code+112),uint32_t(code+96)};
#endif
for(unsigned initial=1;initial<=65536;++initial){frees=0;uint32_t results[5];
#ifdef _WIN64
StudioDecalQueuedCallCandidate call;call.references.store(initial);call.owner=0x100000001ULL;call.argument=0x200000002ULL;results[0]=call.addReference();results[1]=call.releaseReference([&](auto* p){check(p==&call);++frees;});results[2]=call.releaseReference([&](auto* p){check(p==&call);++frees;});results[3]=call.references.load();results[4]=frees;check(call.owner==0x100000001ULL&&call.argument==0x200000002ULL);uint32_t expected[5];check(bool(file.read(reinterpret_cast<char*>(expected),sizeof(expected))));check(std::memcmp(results,expected,sizeof(results))==0);
#else
uint32_t object[9];std::memset(object,0xa5,sizeof(object));object[2]=uint32_t(secondary);object[3]=initial;expectedObject=object;uint32_t before[9];std::memcpy(before,object,sizeof(object));results[0]=add(object);results[1]=release(object);results[2]=release(object);results[3]=object[3];results[4]=frees;before[3]=object[3];check(std::memcmp(before,object,sizeof(object))==0);file.write(reinterpret_cast<char*>(results),sizeof(results));
#endif
}
#ifdef _WIN64
check(file.peek()==std::char_traits<char>::eof());
#else
file.flush();check(bool(file));VirtualFree(code,0,MEM_RELEASE);
#endif
std::cout<<"queued_reference_cases=65536\n";return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
