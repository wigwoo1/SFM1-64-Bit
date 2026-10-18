#include "scalar_reference_storage_sequence.h"
#include <fstream>
#include <array>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{if(argc!=2)return 2;std::ifstream f(argv[1],std::ios::binary);unsigned cases=0;for(unsigned old=0;old<17;++old)for(unsigned incoming=0;incoming<17;++incoming){uint32_t storage=old==16?0xffffffffu:old,newValue=incoming==16?0xffffffffu:incoming;std::array<uint32_t,8>actual{},expected{};unsigned count=0;assignRecoveredScalarReferenceStorageCandidate(storage,newValue,[&](uint32_t target){actual[count++]=1;actual[count++]=target;actual[count++]=storage;},[&](uint32_t target){actual[count++]=2;actual[count++]=target;actual[count++]=storage;});actual[6]=storage;actual[7]=count;f.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!f||actual!=expected)throw std::runtime_error("native scalar sequence mismatch");++cases;}char extra;if(f.read(&extra,1))throw std::runtime_error("trailing oracle");std::cout<<"native_scalar_reference_storage_matches="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
