#pragma once
#include <array>
#include <cstdint>
#include <limits>
struct StudioMaterialNonpositionDescriptorCandidate{
 std::array<uint32_t,8> present{};std::array<uintptr_t,8> pointers{};
 std::array<uint32_t,8> texturePresent{};uintptr_t inlineTextureArrayAddress=0;
};
// Native no-position branch selects unsigned lowest enabled destination.
// Texture branch uses address of descriptor inline array, not its elements.
// Caller must provide at least one valid enabled field; native otherwise unsafe.
template<class Copy>void studioMaterialNonpositionVertexUploadCandidate(const StudioMaterialNonpositionDescriptorCandidate& descriptor,uintptr_t source,uint32_t stride,uint32_t bytes,Copy copy){
 if(!source||!stride)return;auto selected=std::numeric_limits<uintptr_t>::max();
 for(unsigned i=0;i<8;++i)if(descriptor.present[i]&&descriptor.pointers[i]<selected)selected=descriptor.pointers[i];
 for(auto present:descriptor.texturePresent)if(present&&descriptor.inlineTextureArrayAddress<selected)selected=descriptor.inlineTextureArrayAddress;
 if(selected)copy(selected,source,bytes);
}
