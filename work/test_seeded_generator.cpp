#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "studio_seeded_generator_candidate.h"
#if !defined(_WIN64)
#include "seeded_generator_native_init.h"
#endif
static void check(bool b){if(!b)std::abort();}
int main(){
#if !defined(_WIN64)
 std::array<unsigned,32*224+2> raw{};auto state=raw.data()+1;double scale=1.0/65536,correction=4294967296.0;
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code);
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);dll.seekg(seedFileOffset);dll.read(reinterpret_cast<char*>(code),sizeof(seedBytes));check(bool(dll)&&!std::memcmp(code,seedBytes,sizeof(seedBytes)));
 for(const auto& f:seedFixups){uintptr_t p=0;switch(f[1]){case 0x101526e8:p=reinterpret_cast<uintptr_t>(&scale);break;case 0x10144098:p=reinterpret_cast<uintptr_t>(&correction);break;case 0x10198eb0:p=reinterpret_cast<uintptr_t>(state)+880;break;case 0x1019feb0:p=reinterpret_cast<uintptr_t>(state)+880+32*896;break;default:std::abort();}unsigned v=static_cast<unsigned>(p);std::memcpy(code+f[0],&v,4);}
 DWORD old;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old)&&FlushInstructionCache(GetCurrentProcess(),code,4096));std::ofstream output("outputs/native-seeded-generator.bin",std::ios::binary|std::ios::trunc);check(bool(output));
#else
 StudioSeedStateCandidate state;check(reinterpret_cast<uintptr_t>(&state)>UINT32_MAX);std::ifstream input("outputs/native-seeded-generator.bin",std::ios::binary);check(bool(input));
#endif
 unsigned cases=0;
 for(unsigned seed:{0u,1u,0x31415926u,0x7fffffffu,0x80000000u,0xfffffff0u,0xffffffffu,0xbb40e62du}){
 std::array<unsigned,32*224> wire{};
#if !defined(_WIN64)
 raw.fill(0xdeadbeef);reinterpret_cast<void(__cdecl*)(unsigned)>(code)(seed);check(raw.front()==0xdeadbeef&&raw.back()==0xdeadbeef);
 for(unsigned i=0;i<32;++i){auto b=state+i*224;std::memcpy(wire.data()+i*224,b,880);check(b[220]==reinterpret_cast<uintptr_t>(b+92)&&b[221]==reinterpret_cast<uintptr_t>(b+216));check(b[222]==0xdeadbeef&&b[223]==0xdeadbeef);wire[i*224+220]=92;wire[i*224+221]=216;wire[i*224+222]=b[222];wire[i*224+223]=b[223];}
 output.write(reinterpret_cast<char*>(wire.data()),sizeof(wire));check(bool(output));
#else
 for(auto& b:state)b.preservedTail.fill(0xdeadbeef);studioInitializeSeedCandidate(state,seed);
 for(unsigned i=0;i<32;++i){auto& b=state[i];std::memcpy(wire.data()+i*224,b.values.data(),880);check(b.first==&b.values[92]&&b.second==&b.values[216]);check(b.preservedTail[0]==0xdeadbeef&&b.preservedTail[1]==0xdeadbeef);wire[i*224+220]=92;wire[i*224+221]=216;wire[i*224+222]=b.preservedTail[0];wire[i*224+223]=b.preservedTail[1];}
 std::array<unsigned,32*224> original{};input.read(reinterpret_cast<char*>(original.data()),sizeof(original));check(bool(input)&&wire==original);
#endif
 ++cases;
 }
#if !defined(_WIN64)
 check(VirtualFree(code,0,MEM_RELEASE));
#else
 check(input.peek()==std::char_traits<char>::eof());
#endif
 std::printf("seeded_generator_%zu_bit_cases=%u full_state_match=passed\n",sizeof(void*)*8,cases);
}
