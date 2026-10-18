#include <string>
#include "reference_root_mark.h"
#include "root_mark_fixture.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <stdexcept>
struct Element{unsigned index;RecoveredReferenceRegistrationStateCandidate state;};
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("oracle required");std::ifstream file(argv[1],std::ios::binary);for(unsigned seed=0;seed<4096;++seed){auto f=rootFixture(seed);std::array<Element,4>elements{};for(unsigned i=0;i<4;++i)elements[i]={i,{f.masks[i],8}};uint32_t trace=2166136261u;
auto state=[](Element*e)->RecoveredReferenceRegistrationStateCandidate&{return e->state;};auto attributes=[&](Element*e,auto visit){for(unsigned j=0;j<2;++j){unsigned k=e->index*2+j;std::vector<uint32_t> handles(f.handles[k].begin(),f.handles[k].begin()+(j?3:1));visit(f.flags[k],handles);}};
auto resolve=[&](uint32_t h)->Element*{rootTrace(trace,h+10);return h<4?&elements[h]:nullptr;};auto accept=[&](Element*e){rootTrace(trace,e->index+20);return f.accepts[e->index];};markRecoveredReferenceRootCandidate(&elements[0],f.enabled,f.mode,f.context,state,attributes,resolve,accept);
std::array<uint32_t,5>expected{};if(!file.read(reinterpret_cast<char*>(expected.data()),sizeof(expected)))throw std::runtime_error("short oracle");for(unsigned i=0;i<4;++i)if(expected[i]!=(0xa5080000u|elements[i].state.contexts))throw std::runtime_error("mask mismatch seed "+std::to_string(seed));if(expected[4]!=trace)throw std::runtime_error("trace mismatch seed "+std::to_string(seed));}char extra;if(file.get(extra))throw std::runtime_error("extra oracle");std::cout<<"x64_root_mark_graph_cases=4096"<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}

