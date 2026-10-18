#pragma once
#include <cstddef>
#include "studio_material_base_cleanup_candidate.h"
// Valid nonnegative capacities/requests and representable signed arithmetic.
// Extreme native signed-wrap repair is not covered by this candidate.
template<class Allocate,class Reallocate>void studioMaterialStackGrowCandidate(StudioMaterialContextDescriptorCandidate& d,int requested,std::size_t stride,Allocate allocate,Reallocate reallocate){
 if(d.growth<0)return;int target=d.capacity+requested,newCapacity=d.capacity;
 if(d.growth)newCapacity=((target-1)/d.growth+1)*d.growth;
 else{if(!newCapacity)newCapacity=1;while(newCapacity<target)newCapacity*=2;}
 d.capacity=newCapacity;
 d.address=d.address?reallocate(d.address,std::size_t(newCapacity)*stride):allocate(std::size_t(newCapacity)*stride);
}
