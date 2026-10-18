#include "scalar_reference_undo_replay.h"
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("oracle required");std::ifstream input(argv[1],std::ios::binary);if(!input)throw std::runtime_error("missing oracle");
 for(unsigned seed=0;seed<512;++seed){RecoveredScalarReferenceUndoPayloadCandidate r{seed/16,(seed&8)?0xffffffffu:42,(seed&16)?0xffffffffu:seed+100,(seed&32)?0xffffffffu:seed+200};std::array<uint32_t,16>out{},expected{};unsigned count=0;auto emit=[&](uint32_t c,uint32_t arg){out[count++]=c;out[count++]=arg;};
 replayRecoveredScalarReferenceUndoCandidate(r,(seed&64)!=0,[&](uint32_t h){emit(1,h);return (seed&1)?uintptr_t(0x100000001ull):0;},[&](uintptr_t owner,uint32_t name){if(owner!=0x100000001ull)throw std::runtime_error("truncated owner");emit(2,name);return (seed&2)?uintptr_t(0x200000001ull):0;},[&](uintptr_t attribute){if(attribute!=0x200000001ull)throw std::runtime_error("truncated attribute");return (seed&4)?0x21u:1u;},[&](uintptr_t attribute,uint32_t target){if(attribute!=0x200000001ull)throw std::runtime_error("truncated write");emit(3,target);});out[15]=count;input.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!input||out!=expected)throw std::runtime_error("native mismatch");
 }if(input.peek()!=EOF)throw std::runtime_error("trailing oracle");std::cout<<"x64_scalar_replay_cases=512 native_events=matched wide_pointers=passed\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
