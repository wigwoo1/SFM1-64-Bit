#pragma once
#include <cstdint>
#include <array>
struct StudioMaterialContextDescriptorCandidate {uintptr_t address=0;int capacity=0,growth=0,count=0;uintptr_t cache=0;};
template<class Free>void studioMaterialDescriptorCleanupCandidate(StudioMaterialContextDescriptorCandidate& descriptor,Free free){
 descriptor.count=0;
 if(descriptor.growth>=0){if(descriptor.address){free(descriptor.address);descriptor.address=0;}descriptor.capacity=0;}
 descriptor.cache=descriptor.address;
 if(descriptor.growth>=0){if(descriptor.address){free(descriptor.address);descriptor.address=0;}descriptor.capacity=0;}
}
// The eleven sequential base descriptors are supplied in ascending offset order.
template<class Free>void studioMaterialBaseCleanupCandidate(std::array<StudioMaterialContextDescriptorCandidate,11>& sequential,StudioMaterialContextDescriptorCandidate& first,Free free){
 for(std::size_t index=sequential.size();index;--index)studioMaterialDescriptorCleanupCandidate(sequential[index-1],free);
 studioMaterialDescriptorCleanupCandidate(first,free);
}
