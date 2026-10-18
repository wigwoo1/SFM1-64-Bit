#include "scalar_reference_change_guard.h"
#include "scalar_reference_write_sequence.h"
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("oracle required");std::ifstream input(argv[1],std::ios::binary);if(!input)throw std::runtime_error("oracle unavailable");unsigned cases=0;
 for(unsigned old=0;old<17;++old)for(unsigned incoming=0;incoming<17;++incoming)for(unsigned allow=0;allow<2;++allow){uint32_t storage=old==16?0xffffffffu:old,value=incoming==16?0xffffffffu:incoming,flags=0;std::array<uint32_t,19>out{},expected{};unsigned count=0;auto event=[&](uint32_t c,uint32_t h){out[count++]=c;out[count++]=h;out[count++]=storage;};
 if(recoveredScalarReferenceChangeAllowedCandidate(1,storage,value,0xffffffffu,[](uint32_t){return 0xffffffffu;},[](uint32_t,uint32_t){return true;},[&](){event(1,0);return allow!=0;}))writeRecoveredScalarReferenceCandidate(storage,value,[&](){event(2,0);},[&](uint32_t h){event(3,h);},[&](uint32_t h){event(4,h);},[](){},[&](){event(5,1);flags|=0x1000;});
 out[16]=storage;out[17]=count;out[18]=flags;input.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!input||out!=expected)throw std::runtime_error("native mismatch");++cases;
 }if(input.peek()!=EOF)throw std::runtime_error("trailing oracle");std::cout<<"x64_scalar_write_cases="<<cases<<" native_events_storage_flags=matched\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
