#define NOMINMAX
#include <Windows.h>
#include <array>
#include <vector>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "studio_restore_identity_matrix_candidate.h"
static unsigned fastFlag;static const float one=1.0f;

int main(){static_assert(sizeof(void*)==4,"Original code requires x86");
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);
 auto load=[&](const char* path,unsigned address,unsigned size){std::vector<unsigned char> raw(size),actual(size);std::ifstream f(path,std::ios::binary);f.read(reinterpret_cast<char*>(raw.data()),size);if(!f||f.peek()!=std::char_traits<char>::eof())std::abort();dll.clear();dll.seekg(address);dll.read(reinterpret_cast<char*>(actual.data()),size);if(!dll||raw!=actual)std::abort();auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)std::abort();std::memcpy(code,raw.data(),size);return code;};
 // File offsets are recovered from the checked PE reader, supplied in the generated header.
 #include "restore_fill_identity_native_init.h"
 #include "restore_fill_fastpath_native_init.h"
 auto* fill=load("outputs/restore-byte-fill-original.bin",restoreFillOffset,122);auto* identity=load("outputs/restore-identity-matrix-original.bin",restoreIdentityOffset,45);
 auto* fast=load("outputs/restore-fill-fastpath-original.bin",restoreFillFastOffset,restoreFillFastSize);const auto flag=reinterpret_cast<uintptr_t>(&fastFlag);std::memcpy(fill+32,&flag,4);const auto trap=static_cast<unsigned>(reinterpret_cast<uintptr_t>(fast)-reinterpret_cast<uintptr_t>(fill+44));std::memcpy(fill+40,&trap,4);
 const auto displacement=static_cast<unsigned>(reinterpret_cast<uintptr_t>(fill)-reinterpret_cast<uintptr_t>(identity+17));std::memcpy(identity+13,&displacement,4);const auto constant=reinterpret_cast<uintptr_t>(&one);std::memcpy(identity+21,&constant,4);
 DWORD old;for(auto* code:{fill,identity,fast}){if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 1;FlushInstructionCache(GetCurrentProcess(),code,4096);}
 using Fill=void*(__cdecl*)(void*,unsigned,unsigned);auto function=reinterpret_cast<Fill>(fill);if(function(nullptr,123,0)!=nullptr)return 2;
 unsigned cases=0;for(unsigned flagValue:{0u,1u})for(unsigned offset=0;offset<16;++offset)for(unsigned count=0;count<=513;++count)for(unsigned value:{0u,1u,255u,256u,0xabcdefu}){
  fastFlag=flagValue;std::array<unsigned char,576> actual{},expected{};actual.fill(0xcc);expected.fill(0xcc);std::memset(expected.data()+offset,static_cast<int>(value&255),count);
  if(function(actual.data()+offset,value,count)!=actual.data()+offset||actual!=expected)return 3;++cases;
 }
 for(unsigned pattern=0;pattern<256;++pattern){std::array<unsigned char,64> actual{},expected{};actual.fill(static_cast<unsigned char>(pattern));expected=actual;std::array<float,12> matrix;studioRestoreIdentityMatrixCandidate(matrix);std::memcpy(expected.data()+8,matrix.data(),48);reinterpret_cast<void(__cdecl*)(void*)>(identity)(actual.data()+8);if(actual!=expected)return 4;}
 VirtualFree(fast,0,MEM_RELEASE);VirtualFree(fill,0,MEM_RELEASE);VirtualFree(identity,0,MEM_RELEASE);std::printf("restore_scalar_fill_cases=%u identity_cases=256 passed actual_fastpath_executed\n",cases);
}

