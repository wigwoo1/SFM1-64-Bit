#include <cstdlib>
#include <iostream>
#include "studio_material_stack_growth_candidate.h"
int main(){unsigned allocations=0,reallocations=0;
auto allocate=[&](size_t bytes){++allocations;return reinterpret_cast<uintptr_t>(std::malloc(bytes));};auto reallocate=[&](uintptr_t old,size_t bytes){++reallocations;return reinterpret_cast<uintptr_t>(std::realloc(reinterpret_cast<void*>(old),bytes));};
for(size_t stride: {size_t(36),size_t(68)})for(int growth: {0,16}){StudioMaterialContextDescriptorCandidate d;d.growth=growth;
for(int i=0;i<513;++i){if(d.count==d.capacity)studioMaterialStackGrowCandidate(d,1,stride,allocate,reallocate);if(!d.address||d.capacity<=i)return 1;auto* bytes=reinterpret_cast<unsigned char*>(d.address);for(int j=0;j<i;++j)if(bytes[size_t(j)*stride]!=static_cast<unsigned char>(j%251))return 1;bytes[size_t(i)*stride]=static_cast<unsigned char>(i%251);++d.count;d.cache=d.address;}
if(d.capacity!=(growth?528:1024))return 1;studioMaterialDescriptorCleanupCandidate(d,[](uintptr_t address){std::free(reinterpret_cast<void*>(address));});if(d.address||d.count||d.capacity)return 1;
}StudioMaterialContextDescriptorCandidate external{0x100000001ULL,1,-1,1,0x100000001ULL};studioMaterialStackGrowCandidate(external,1,68,allocate,reallocate);if(external.capacity!=1||external.address!=0x100000001ULL)return 1;
std::cout<<"stack_growth_paths=4 retained_elements=2052 allocations="<<allocations<<" reallocations="<<reallocations<<" external_noop=1\n";}
