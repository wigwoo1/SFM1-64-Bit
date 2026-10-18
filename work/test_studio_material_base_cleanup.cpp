#include <memory>
#include <vector>
#include <iostream>
#include "studio_material_base_cleanup_candidate.h"
int main(){unsigned frees=0;
for(unsigned mask=0;mask<4096;++mask){std::array<std::unique_ptr<unsigned>,12> storage;std::array<StudioMaterialContextDescriptorCandidate,11> descriptors;StudioMaterialContextDescriptorCandidate first;std::vector<unsigned> order;
for(unsigned i=0;i<12;++i){storage[i]=std::make_unique<unsigned>(i);auto& d=i<11?descriptors[i]:first;d={reinterpret_cast<uintptr_t>(storage[i].get()),1,(mask&(1u<<i))?-1:0,1,0};}
studioMaterialBaseCleanupCandidate(descriptors,first,[&](uintptr_t address){auto* p=reinterpret_cast<unsigned*>(address);unsigned index=*p;if(storage[index].get()!=p)throw 1;order.push_back(index);storage[index].reset();++frees;});
std::vector<unsigned> expected;for(int i=10;i>=0;--i)if(!(mask&(1u<<i)))expected.push_back(unsigned(i));if(!(mask&(1u<<11)))expected.push_back(11);if(order!=expected)return 1;
for(unsigned i=0;i<12;++i){auto& d=i<11?descriptors[i]:first;bool external=(mask&(1u<<i))!=0;if(d.count||bool(d.address)!=external||d.capacity!=int(external)||d.cache!=d.address||bool(storage[i])!=external)return 1;}
}std::cout<<"base_cleanup_ownership_cases=4096 actual_owned_frees="<<frees<<" external_buffers_retained=1\n";}
