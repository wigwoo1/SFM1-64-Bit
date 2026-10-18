#include "scalar_reference_change_guard.h"
#include <fstream>
#include <array>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{if(argc!=2)return 2;std::ifstream f(argv[1],std::ios::binary);unsigned cases=0;for(unsigned type=0;type<2;++type)for(unsigned old=0;old<16;++old)for(unsigned incoming=0;incoming<16;++incoming)for(unsigned allow=0;allow<2;++allow){unsigned calls=0,resolves=0;auto resolve=[&](uint32_t){++resolves;return 0xffffffffu;};auto match=[](uint32_t,uint32_t){return false;};uint32_t result=recoveredScalarReferenceChangeAllowedCandidate(type?3:1,old,incoming,0xffffffffu,resolve,match,[&](){++calls;return allow!=0;});std::array<uint32_t,2>actual{result,calls},expected{};f.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!f||resolves||actual!=expected)throw std::runtime_error("native scalar guard mismatch");++cases;}char extra;if(f.read(&extra,1))throw std::runtime_error("trailing oracle");std::cout<<"native_scalar_reference_guard_matches="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
