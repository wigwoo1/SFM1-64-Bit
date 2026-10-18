#define NOMINMAX
#include <Windows.h>
#include "color_generator_native_fixups.h"
#include "studio_color_table_candidate.h"
#include <fstream>
#include <iterator>
#include <vector>
#include <cstdio>
int main(){
 static_assert(sizeof(void*)==4,"x86 oracle");
 std::ifstream input("work/color-generator-native-image.bin",std::ios::binary);
 std::vector<unsigned char> image((std::istreambuf_iterator<char>(input)),{});
 if(image.size()!=colorGeneratorImageSize)return 1;
 uint64_t imageDigest=1469598103934665603ull;for(auto b:image)imageDigest=(imageDigest^b)*1099511628211ull;
 if(imageDigest!=colorGeneratorImageDigest)return 2;
 auto* memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,image.size(),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(!memory)return 3;
 std::memcpy(memory,image.data(),image.size());
 for(auto& f:colorGeneratorFixups){uint32_t target=reinterpret_cast<uintptr_t>(memory+f[1]);std::memcpy(memory+f[0],&target,4);}
 DWORD old=0;
 if(!VirtualProtect(memory,image.size(),PAGE_NOACCESS,&old)
 ||!VirtualProtect(memory,0x2000,PAGE_EXECUTE_READ,&old)
 ||!VirtualProtect(memory+0x4000,0x7000,PAGE_READONLY,&old)
 ||!VirtualProtect(memory+0xc000,0x1000,PAGE_EXECUTE_READ,&old)
 ||!VirtualProtect(memory+0x10000,0x9000,PAGE_READWRITE,&old)
 ||!VirtualProtect(memory+0x19000,image.size()-0x19000,PAGE_READONLY,&old))return 4;
 FlushInstructionCache(GetCurrentProcess(),memory,image.size());
 float gamma;std::memcpy(&gamma,&colorGeneratorGammaBits,4);
 auto generator=reinterpret_cast<void(__cdecl*)(float,float,float,int)>(memory+0xc020);
 generator(gamma,gamma,0.f,2);
 const auto candidate=studioColorTableCandidate();
 auto* native=reinterpret_cast<const float*>(memory+colorGeneratorLookupOffset);
 unsigned mismatches=0;uint64_t digest=1469598103934665603ull;
 for(unsigned i=0;i<256;++i){uint32_t got,expected;std::memcpy(&got,native+i,4);std::memcpy(&expected,&candidate[i],4);if(got!=expected)++mismatches;digest=(digest^got)*1099511628211ull;}
 std::ofstream output("outputs/native-full-generator-color-table.bin",std::ios::binary);output.write(reinterpret_cast<const char*>(native),1024);if(!output)return 5;
 VirtualFree(memory,0,MEM_RELEASE);
 std::printf("full_native_generator_lookup_entries=256 bit_mismatches=%u digest=%llu initialization_gamma=%g other_tables_parity=unverified\n",mismatches,static_cast<unsigned long long>(digest),gamma);return mismatches?6:0;
}
