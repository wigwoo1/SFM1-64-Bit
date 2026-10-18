#include <memory>
#include <unordered_map>
#include <vector>
#include <iostream>
#include "studio_material_base_stack_construction_candidate.h"
int main(){unsigned frees=0;
for(unsigned cycle=0;cycle<256;++cycle){StudioMaterialBaseStacksCandidate stacks;std::unordered_map<uintptr_t,std::unique_ptr<unsigned char[]>> allocations;std::vector<uintptr_t> order;
auto allocate=[&](size_t bytes){auto data=std::make_unique<unsigned char[]>(bytes);std::memset(data.get(),int(cycle),bytes);auto address=reinterpret_cast<uintptr_t>(data.get());allocations.emplace(address,std::move(data));order.push_back(address);return address;};
studioMaterialBaseStacksConstructCandidate(stacks,allocate,[](uintptr_t,size_t)->uintptr_t{throw 1;});
if(allocations.size()!=12||stacks.state.capacity!=32||stacks.state.count!=1||stacks.state.growth!=16||stacks.state.cache!=stacks.state.address||stacks.currentMatrix!=stacks.matrices[0].address)return 1;
uint32_t state[9];std::memcpy(state,reinterpret_cast<void*>(stacks.state.address),sizeof(state));for(unsigned i=0;i<9;++i)if(state[i]!=(i<7?0u:0xffffffffu))return 1;
for(auto& d:stacks.matrices){if(d.capacity!=1||d.count!=1||d.growth||d.cache!=d.address)return 1;float values[16];std::memcpy(values,reinterpret_cast<void*>(d.address),sizeof(values));for(unsigned i=0;i<16;++i)if(values[i]!=(i%5==0?1.f:0.f))return 1;uint32_t flags;std::memcpy(&flags,reinterpret_cast<void*>(d.address+64),4);if(flags!=((cycle*0x01010101u)|3))return 1;}
std::vector<uintptr_t> freed;studioMaterialBaseCleanupCandidate(stacks.matrices,stacks.state,[&](uintptr_t address){if(allocations.erase(address)!=1)throw 1;freed.push_back(address);++frees;});
if(!allocations.empty()||freed.size()!=12)return 1;for(unsigned i=0;i<11;++i)if(freed[i]!=order[11-i])return 1;if(freed.back()!=order[0])return 1;
}std::cout<<"base_stack_lifetimes=256 identity_stacks=2816 actual_frees="<<frees<<" retained_flag_patterns=256\n";}
