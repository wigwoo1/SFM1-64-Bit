#include "reference_cleanup_root_phases.h"
#include "reference_registration_state.h"
#include "reference_runtime_element.h"
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("oracle required");std::ifstream f(argv[1],std::ios::binary);for(unsigned pattern=0;pattern<512;++pattern){RecoveredReferenceRuntimeElementCandidate e{};e.cleanupContext=pattern&2?0:0xffffffffu;e.record.counts=pattern&4?0x100000:0;RecoveredReferenceRegistrationStateCandidate s{uint16_t((pattern>>7)&3),8};std::array<uint32_t,35>actual{},expected{};
auto raw=[&](auto visit){if((pattern&1)&&!(pattern&8))visit(e);};auto state=[&](auto&)->RecoveredReferenceRegistrationStateCandidate&{return s;};auto mark=[&](auto&,uint32_t a,uint32_t b,uint32_t c){unsigned i=actual[2]++;actual[3+i*4]=1;actual[4+i*4]=a;actual[5+i*4]=b;actual[6+i*4]=c;s.setEligible(true,c);};auto deps=[&](auto visit){if(!(pattern&64))visit(1);visit(4);};auto resolve=[&](uint32_t h){return (pattern&32)&&h==1?&e:nullptr;};recoveredReferenceCleanupRootPhasesCandidate(raw,state,mark,deps,resolve);actual[0]=0xa5080000u|s.contexts;if(!f.read(reinterpret_cast<char*>(expected.data()),sizeof(expected)))throw std::runtime_error("short oracle");if(actual!=expected)throw std::runtime_error("native mismatch");}char extra;if(f.get(extra))throw std::runtime_error("extra oracle");std::cout<<"x64_cleanup_root_phase_cases=512"<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
