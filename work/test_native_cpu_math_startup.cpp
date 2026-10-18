#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vector>
#include "studio_owned_math_startup_candidate.h"
#include "studio_native_math_cpu_candidate.h"
static void check(bool b){if(!b)std::abort();}
struct Host {unsigned cpu=0,diagnostics=0;unsigned cpuFlags(){++cpu;return studioNativeMathCpuFlagsCandidate();}void mathDiagnostic(const char*){++diagnostics;}};
int main(){check(studioNativeMathCpuFlagsCandidate()==24);StudioMathTablesOwnerCandidate owner;Host host;StudioOwnedMathStartupOpsCandidate<Host> ops{host,owner};studioMathLibraryInitializeCandidate(owner.startup,2.2f,2.2f,0.0f,2,ops);check(owner.startup.initialized&&host.cpu==1&&host.diagnostics==0);check(reinterpret_cast<uintptr_t>(&owner)>UINT32_MAX);
 std::ifstream trig("outputs/native-actual-sine-table.bin",std::ios::binary);std::array<unsigned,768> reference{};trig.read(reinterpret_cast<char*>(reference.data()),sizeof(reference));check(bool(trig));for(unsigned i=0;i<256;++i)check(!std::memcmp(&owner.trigonometric[i],&reference[3*i+2],4));check(trig.peek()==std::char_traits<char>::eof());
 // Gamma oracle case: gamma index1, texture index1, brightness index1, overbright index1 =>52.
 constexpr size_t argumentBytes=1536*8+5632*16;
 constexpr size_t tableBytes=256+1024*4+3*256*4+1024*4+4096*4+4096;
 std::ifstream gamma("outputs/native-actual-power-gamma-startup.bin",std::ios::binary);gamma.seekg(52*(argumentBytes+tableBytes)+argumentBytes);std::vector<unsigned char> original(tableBytes);gamma.read(reinterpret_cast<char*>(original.data()),original.size());check(bool(gamma));size_t offset=0;
 const auto match=[&](const auto& table){check(!std::memcmp(original.data()+offset,table.data(),sizeof(table)));offset+=sizeof(table);};
 match(owner.gamma.textureGamma);match(owner.gamma.linearToScreenGamma);match(owner.gamma.textureToLinear);match(owner.gamma.fixedPower0454);match(owner.gamma.fixedPower22);match(owner.gamma.linearToTexture);match(owner.gamma.linearToVertex);match(owner.gamma.linearToLightmap);check(offset==tableBytes);
 std::ifstream seed("outputs/native-seeded-generator.bin",std::ios::binary);seed.seekg(2*32*224*4);std::array<unsigned,32*224> seedRef{};seed.read(reinterpret_cast<char*>(seedRef.data()),sizeof(seedRef));check(bool(seed));for(unsigned i=0;i<32;++i){auto& b=owner.seeded[i];check(!std::memcmp(b.values.data(),seedRef.data()+i*224,880));check(b.first==&b.values[92]&&b.second==&b.values[216]);}
 owner.gamma.textureGamma[17]=123;owner.trigonometric[19]=42;owner.seeded[0].values[0]=99;studioMathLibraryInitializeCandidate(owner.startup,1.0f,1.0f,1.5f,4,ops);check(host.cpu==1&&owner.gamma.textureGamma[17]==123&&owner.trigonometric[19]==42&&owner.seeded[0].values[0]==99);
 std::puts("x64_native_cpu_owned_math_startup_tables_and_repeat_guard=passed");
}


