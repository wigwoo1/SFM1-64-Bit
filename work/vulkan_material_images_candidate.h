#pragma once
#include "vulkan_material_descriptor_counts_candidate.h"
#include "material_texture_upload_plan_candidate.h"
// Owned diagnostic textures; resources live through submission and readback.
struct VulkanMaterialImagesCandidate {
 HMODULE library;VkDevice device;VkPhysicalDevice physical;VkDescriptorPool pool=VK_NULL_HANDLE;VkSampler sampler=VK_NULL_HANDLE;
 struct Entry{VkImage image=VK_NULL_HANDLE;VkImageView view=VK_NULL_HANDLE;VkBuffer staging=VK_NULL_HANDLE;VkDeviceMemory imageMemory=VK_NULL_HANDLE,stagingMemory=VK_NULL_HANDLE;VkDescriptorSet set=VK_NULL_HANDLE;uint32_t width=0,height=0;uint32_t mipLevels=1;VkFormat format=VK_FORMAT_R8G8B8A8_UNORM;std::vector<VkBufferImageCopy>uploadRegions;};
 std::vector<Entry>entries,maskEntries,secondEntries;
 VulkanMaterialImagesCandidate(HMODULE l,VkDevice d,VkPhysicalDevice p):library(l),device(d),physical(p){}
 VulkanMaterialImagesCandidate(const VulkanMaterialImagesCandidate&)=delete;
 ~VulkanMaterialImagesCandidate(){if(pool)load<PFN_vkDestroyDescriptorPool>(library,"vkDestroyDescriptorPool")(device,pool,nullptr);if(sampler)load<PFN_vkDestroySampler>(library,"vkDestroySampler")(device,sampler,nullptr);for(auto*collection:{&entries,&maskEntries,&secondEntries})for(auto&e:*collection){if(e.view)load<PFN_vkDestroyImageView>(library,"vkDestroyImageView")(device,e.view,nullptr);if(e.image)load<PFN_vkDestroyImage>(library,"vkDestroyImage")(device,e.image,nullptr);if(e.staging)load<PFN_vkDestroyBuffer>(library,"vkDestroyBuffer")(device,e.staging,nullptr);if(e.imageMemory)load<PFN_vkFreeMemory>(library,"vkFreeMemory")(device,e.imageMemory,nullptr);if(e.stagingMemory)load<PFN_vkFreeMemory>(library,"vkFreeMemory")(device,e.stagingMemory,nullptr);}}
 static void check(VkResult r){if(r!=VK_SUCCESS)throw std::runtime_error("material Vulkan result "+std::to_string(r));}
 // Opt-in ordinary texture upload. One entry per animation frame, all validated
 // mips in each image. Empty owner required: replacing in-flight resources needs
 // caller submission/fence ownership. No descriptor/cache/material publication.
 // compressionBcEnabled is the caller's explicit logical-device feature promise.
 void createUploadPlan(const MaterialTextureUploadPlanCandidate&plan,bool srgb=false,bool compressionBcEnabled=false,VkImageUsageFlags extraUsage=0){
  if(pool||sampler||!entries.empty()||!maskEntries.empty()||!secondEntries.empty())throw std::runtime_error("material images already created");
  const uint32_t source=plan.sourceFormat(),mips=plan.mipCount(),frames=plan.frameCount();
  if(!frames||!mips||source!=plan.destinationFormat())throw std::runtime_error("material upload needs a complete same-format ordinary plan");
  const bool compressed=source==13||source==15;
  if(compressed&&!compressionBcEnabled)throw std::runtime_error("BC compression feature not enabled by caller");
  VkFormat format=VK_FORMAT_UNDEFINED;
  if(source==0||source==3)format=srgb?VK_FORMAT_R8G8B8A8_SRGB:VK_FORMAT_R8G8B8A8_UNORM;
  else if(source==12)format=srgb?VK_FORMAT_B8G8R8A8_SRGB:VK_FORMAT_B8G8R8A8_UNORM;
  else if(source==13)format=srgb?VK_FORMAT_BC1_RGBA_SRGB_BLOCK:VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
  else if(source==15)format=srgb?VK_FORMAT_BC3_SRGB_BLOCK:VK_FORMAT_BC3_UNORM_BLOCK;
  else throw std::runtime_error("material upload source format unsupported");
  struct Packed{uint32_t width=0,height=0;std::vector<uint8_t>bytes;std::vector<VkBufferImageCopy>regions;};std::vector<Packed>packed(frames);
  size_t at=0,totalPackedBytes=0;const auto&commands=plan.commands();
  for(uint32_t frame=0;frame<frames;++frame){
   if(at==commands.size()||commands[at].kind!=MaterialTextureUploadCommandKindCandidate::SelectFrame||commands[at].frame!=frame)throw std::runtime_error("material upload frame order");++at;auto&p=packed[frame];
   for(uint32_t mip=0;mip<mips;++mip){
    if(at==commands.size())throw std::runtime_error("material upload mip coverage");const auto&c=commands[at++];const auto&a=c.arguments;
    if(c.kind!=MaterialTextureUploadCommandKindCandidate::Upload2D||c.frame!=frame||a[0]!=mip||a[1]||a[2]!=source||a[3]||a[6]!=source||a[7]||!c.pixels||!c.pixels->data())throw std::runtime_error("material upload ordinary mip command");
    const uint32_t w=a[4],h=a[5];if(!mip){p.width=w;p.height=h;}
    uint32_t maxMips=1;for(auto n=std::max(p.width,p.height);n>1;n>>=1)++maxMips;
    if(!w||!h||mips>maxMips||w!=std::max(1u,p.width>>mip)||h!=std::max(1u,p.height>>mip))throw std::runtime_error("material upload mip dimensions");
    const uint64_t inputBytes=compressed?uint64_t((w+3)/4)*((h+3)/4)*(source==15?16u:8u):uint64_t(w)*h*(source==3?3u:4u);
    if(inputBytes!=c.pixels->size())throw std::runtime_error("material upload mip byte extent");
    const size_t alignment=compressed?(source==15?16u:8u):4u,offset=(p.bytes.size()+alignment-1)&~(alignment-1);
    const uint64_t count=source==3?uint64_t(w)*h*4:inputBytes;if(count>(size_t(1)<<29)||offset>(size_t(1)<<29)-count)throw std::runtime_error("material upload staging capacity");
    p.bytes.resize(offset+size_t(count),0);const auto*data=c.pixels->data();
    if(source==3){for(size_t i=0;i<size_t(w)*h;++i){p.bytes[offset+i*4]=data[i*3+2];p.bytes[offset+i*4+1]=data[i*3+1];p.bytes[offset+i*4+2]=data[i*3];p.bytes[offset+i*4+3]=255;}}
    else std::memcpy(p.bytes.data()+offset,data,size_t(count));
    VkBufferImageCopy region{};region.bufferOffset=offset;region.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,mip,0,1};region.imageExtent={w,h,1};p.regions.push_back(region);
   }
   if(frame&&(p.width!=packed[0].width||p.height!=packed[0].height))throw std::runtime_error("material upload frame dimensions");
   if(totalPackedBytes>(size_t(1)<<29)-p.bytes.size())throw std::runtime_error("material upload total staging capacity");totalPackedBytes+=p.bytes.size();
  }
  if(at!=commands.size())throw std::runtime_error("material upload extra commands");
  const VkImageUsageFlags usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|extraUsage;VkImageFormatProperties limits{};
  check(load<PFN_vkGetPhysicalDeviceImageFormatProperties>(library,"vkGetPhysicalDeviceImageFormatProperties")(physical,format,VK_IMAGE_TYPE_2D,VK_IMAGE_TILING_OPTIMAL,usage,0,&limits));
  VkFormatProperties formats{};load<PFN_vkGetPhysicalDeviceFormatProperties>(library,"vkGetPhysicalDeviceFormatProperties")(physical,format,&formats);
  if(!(formats.optimalTilingFeatures&VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)||packed[0].width>limits.maxExtent.width||packed[0].height>limits.maxExtent.height||mips>limits.maxMipLevels)throw std::runtime_error("material upload device format/extent unavailable");
  VkPhysicalDeviceMemoryProperties properties{};load<PFN_vkGetPhysicalDeviceMemoryProperties>(library,"vkGetPhysicalDeviceMemoryProperties")(physical,&properties);
  auto allocate=[&](VkMemoryRequirements req,VkMemoryPropertyFlags flags,VkDeviceMemory&out){uint32_t type=UINT32_MAX;for(uint32_t n=0;n<properties.memoryTypeCount;++n)if((req.memoryTypeBits&(1u<<n))&&(properties.memoryTypes[n].propertyFlags&flags)==flags){type=n;break;}if(type==UINT32_MAX)throw std::runtime_error("material upload memory type");VkMemoryAllocateInfo info{};info.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;info.allocationSize=req.size;info.memoryTypeIndex=type;check(load<PFN_vkAllocateMemory>(library,"vkAllocateMemory")(device,&info,nullptr,&out));};
  VulkanMaterialImagesCandidate pending(library,device,physical);pending.entries.resize(frames);
  for(uint32_t frame=0;frame<frames;++frame){auto&e=pending.entries[frame];const auto&p=packed[frame];e.width=p.width;e.height=p.height;e.mipLevels=mips;e.format=format;e.uploadRegions=p.regions;
   VkBufferCreateInfo bi{};bi.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;bi.size=p.bytes.size();bi.usage=VK_BUFFER_USAGE_TRANSFER_SRC_BIT;check(load<PFN_vkCreateBuffer>(library,"vkCreateBuffer")(device,&bi,nullptr,&e.staging));VkMemoryRequirements req{};load<PFN_vkGetBufferMemoryRequirements>(library,"vkGetBufferMemoryRequirements")(device,e.staging,&req);allocate(req,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,e.stagingMemory);check(load<PFN_vkBindBufferMemory>(library,"vkBindBufferMemory")(device,e.staging,e.stagingMemory,0));void*mapped=nullptr;check(load<PFN_vkMapMemory>(library,"vkMapMemory")(device,e.stagingMemory,0,p.bytes.size(),0,&mapped));std::memcpy(mapped,p.bytes.data(),p.bytes.size());load<PFN_vkUnmapMemory>(library,"vkUnmapMemory")(device,e.stagingMemory);
   VkImageCreateInfo ii{};ii.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;ii.imageType=VK_IMAGE_TYPE_2D;ii.format=format;ii.extent={e.width,e.height,1};ii.mipLevels=mips;ii.arrayLayers=1;ii.samples=VK_SAMPLE_COUNT_1_BIT;ii.tiling=VK_IMAGE_TILING_OPTIMAL;ii.usage=usage;check(load<PFN_vkCreateImage>(library,"vkCreateImage")(device,&ii,nullptr,&e.image));load<PFN_vkGetImageMemoryRequirements>(library,"vkGetImageMemoryRequirements")(device,e.image,&req);allocate(req,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,e.imageMemory);check(load<PFN_vkBindImageMemory>(library,"vkBindImageMemory")(device,e.image,e.imageMemory,0));VkImageViewCreateInfo vi{};vi.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;vi.image=e.image;vi.viewType=VK_IMAGE_VIEW_TYPE_2D;vi.format=format;vi.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,mips,0,1};check(load<PFN_vkCreateImageView>(library,"vkCreateImageView")(device,&vi,nullptr,&e.view));
  }
  entries.swap(pending.entries);
 }
 void create(const std::vector<ModelRenderMaterialCandidate>&materials,VkDescriptorSetLayout layout,bool withSelfIllumMask=false,bool baseSrgb=false,bool useMaterialColorSpace=false,bool independentSamplers=false){
  createFrom(materials.size(),[&](size_t n)->const ModelRenderMaterialCandidate&{return materials.at(n);},layout,withSelfIllumMask,baseSrgb,useMaterialColorSpace,0,independentSamplers);
 }
 template<class Scene,class Inputs>
 void createScene(const Scene&scene,const Inputs&inputs,VkDescriptorSetLayout layout,bool withSelfIllumMask=false,bool baseSrgb=false,bool useMaterialColorSpace=false,bool independentSamplers=false){
  createFrom(scene.materials.size(),[&](size_t n)->const ModelRenderMaterialCandidate&{const auto&address=scene.materials.at(n);return inputs.at(address.model).materials.at(address.material);},layout,withSelfIllumMask,baseSrgb,useMaterialColorSpace,0,independentSamplers);
 }
 // Shared owned upload allocation path for base and future selfillum mask images.
 // Entry must already belong to this owner so partial failures are cleaned up.
 template<class Image,class Allocate>
 void createOwnedImage(const Image&image,Entry&e,Allocate&allocate,VkFormat format=VK_FORMAT_R8G8B8A8_UNORM,VkImageUsageFlags extraUsage=0){
  if(format!=VK_FORMAT_R8G8B8A8_UNORM&&format!=VK_FORMAT_R8G8B8A8_SRGB)throw std::runtime_error("unsupported material sampling format");
  if(e.image||e.view||e.staging||e.imageMemory||e.stagingMemory)throw std::runtime_error("material image entry already allocated");
  if(!image.width||!image.height||uint64_t(image.width)*image.height>SIZE_MAX/4||image.rgba.size()!=uint64_t(image.width)*image.height*4)throw std::runtime_error("owned material image dimensions or bytes");
  e.width=image.width;e.height=image.height;e.format=format;VkBufferCreateInfo bi{};bi.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;bi.size=image.rgba.size();bi.usage=VK_BUFFER_USAGE_TRANSFER_SRC_BIT;check(load<PFN_vkCreateBuffer>(library,"vkCreateBuffer")(device,&bi,nullptr,&e.staging));VkMemoryRequirements req{};load<PFN_vkGetBufferMemoryRequirements>(library,"vkGetBufferMemoryRequirements")(device,e.staging,&req);allocate(req,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,e.stagingMemory);check(load<PFN_vkBindBufferMemory>(library,"vkBindBufferMemory")(device,e.staging,e.stagingMemory,0));void*mapped=nullptr;check(load<PFN_vkMapMemory>(library,"vkMapMemory")(device,e.stagingMemory,0,image.rgba.size(),0,&mapped));std::memcpy(mapped,image.rgba.data(),image.rgba.size());load<PFN_vkUnmapMemory>(library,"vkUnmapMemory")(device,e.stagingMemory);
   VkImageCreateInfo ii{};ii.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;ii.imageType=VK_IMAGE_TYPE_2D;ii.format=format;ii.extent={e.width,e.height,1};ii.mipLevels=ii.arrayLayers=1;ii.samples=VK_SAMPLE_COUNT_1_BIT;ii.tiling=VK_IMAGE_TILING_OPTIMAL;ii.usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|extraUsage;check(load<PFN_vkCreateImage>(library,"vkCreateImage")(device,&ii,nullptr,&e.image));load<PFN_vkGetImageMemoryRequirements>(library,"vkGetImageMemoryRequirements")(device,e.image,&req);allocate(req,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,e.imageMemory);check(load<PFN_vkBindImageMemory>(library,"vkBindImageMemory")(device,e.image,e.imageMemory,0));VkImageViewCreateInfo vi{};vi.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;vi.image=e.image;vi.viewType=VK_IMAGE_VIEW_TYPE_2D;vi.format=ii.format;vi.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};check(load<PFN_vkCreateImageView>(library,"vkCreateImageView")(device,&vi,nullptr,&e.view));
 }
 template<class Material>
 void createFrom(size_t materialCount,Material material,VkDescriptorSetLayout layout,bool withSelfIllumMask=false,bool baseSrgb=false,bool useMaterialColorSpace=false,VkImageUsageFlags extraUsage=0,bool independentSamplers=false,bool lightwarpLayout=false){
   if(pool||sampler||!entries.empty()||!maskEntries.empty()||!secondEntries.empty())throw std::runtime_error("material images already created");
  // Budget every declared image binding, even when unused. The normal editor
  // opts into samplers1/4/5; legacy smaller layouts retain the sampler1 default.
  // Decoded channels currently share the same owned repeat/linear policy, but
  // each declared sampler binding is initialized before prepared replacements.
  const auto descriptorCounts=vulkanMaterialDescriptorCountsCandidate(materialCount,true,independentSamplers,lightwarpLayout);
  for(size_t n=0;n<materialCount;++n){const auto&image=material(n).image;if(!image.width||!image.height||uint64_t(image.width)*image.height>SIZE_MAX/4||image.rgba.size()!=uint64_t(image.width)*image.height*4)throw std::runtime_error("material image dimensions or bytes");}
  // Binding2 requires the caller to supply a compatible three-binding layout.
  if(withSelfIllumMask)for(size_t n=0;n<materialCount;++n){const auto&m=material(n);if(m.selfIllum.enabled&&!m.selfIllum.mask.empty()){const auto&image=m.selfIllumMaskImage;if(!image.width||!image.height||uint64_t(image.width)*image.height>SIZE_MAX/4||image.rgba.size()!=uint64_t(image.width)*image.height*4)throw std::runtime_error("selfillum mask dimensions or bytes");}}
   for(size_t n=0;n<materialCount;++n){const auto&image=material(n).secondBaseImage;if(!image.rgba.empty()&&(!image.width||!image.height||uint64_t(image.width)*image.height>SIZE_MAX/4||image.rgba.size()!=uint64_t(image.width)*image.height*4))throw std::runtime_error("second base image dimensions or bytes");}
  if(!materialCount)return;
   entries.resize(materialCount);secondEntries.resize(materialCount);if(withSelfIllumMask)maskEntries.resize(materialCount);VkPhysicalDeviceMemoryProperties properties{};load<PFN_vkGetPhysicalDeviceMemoryProperties>(library,"vkGetPhysicalDeviceMemoryProperties")(physical,&properties);
  auto allocate=[&](VkMemoryRequirements req,VkMemoryPropertyFlags flags,VkDeviceMemory&out){uint32_t type=UINT32_MAX;for(uint32_t n=0;n<properties.memoryTypeCount;++n)if((req.memoryTypeBits&(1u<<n))&&(properties.memoryTypes[n].propertyFlags&flags)==flags){type=n;break;}if(type==UINT32_MAX)throw std::runtime_error("material memory type");VkMemoryAllocateInfo info{};info.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;info.allocationSize=req.size;info.memoryTypeIndex=type;check(load<PFN_vkAllocateMemory>(library,"vkAllocateMemory")(device,&info,nullptr,&out));};
  VkSamplerCreateInfo si{};si.sType=VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;si.magFilter=si.minFilter=VK_FILTER_LINEAR;si.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;si.addressModeU=si.addressModeV=si.addressModeW=VK_SAMPLER_ADDRESS_MODE_REPEAT;check(load<PFN_vkCreateSampler>(library,"vkCreateSampler")(device,&si,nullptr,&sampler));
   VkDescriptorPoolSize sizes[2]{{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,descriptorCounts.sampledImages},{VK_DESCRIPTOR_TYPE_SAMPLER,descriptorCounts.samplers}};VkDescriptorPoolCreateInfo pi{};pi.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;pi.maxSets=descriptorCounts.maxSets;pi.poolSizeCount=2;pi.pPoolSizes=sizes;check(load<PFN_vkCreateDescriptorPool>(library,"vkCreateDescriptorPool")(device,&pi,nullptr,&pool));
   for(size_t n=0;n<entries.size();++n){auto&e=entries[n];const auto&image=material(n).image;bool selectedSrgb=useMaterialColorSpace?material(n).colorSpace.baseSrgb:baseSrgb;createOwnedImage(image,e,allocate,selectedSrgb?VK_FORMAT_R8G8B8A8_SRGB:VK_FORMAT_R8G8B8A8_UNORM,extraUsage);if(withSelfIllumMask){const auto&m=material(n);if(m.selfIllum.enabled&&!m.selfIllum.mask.empty())createOwnedImage(m.selfIllumMaskImage,maskEntries[n],allocate);else{VtfRgbaImageCandidate black{1,1,{0,0,0,255}};createOwnedImage(black,maskEntries[n],allocate);}}const auto&second=material(n).secondBaseImage;if(second.rgba.empty()){VtfRgbaImageCandidate black{1,1,{0,0,0,255}};createOwnedImage(black,secondEntries[n],allocate);}else createOwnedImage(second,secondEntries[n],allocate,selectedSrgb?VK_FORMAT_R8G8B8A8_SRGB:VK_FORMAT_R8G8B8A8_UNORM);
    VkDescriptorSetAllocateInfo ai{};ai.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;ai.descriptorPool=pool;ai.descriptorSetCount=1;ai.pSetLayouts=&layout;check(load<PFN_vkAllocateDescriptorSets>(library,"vkAllocateDescriptorSets")(device,&ai,&e.set));VkDescriptorImageInfo images[4]{{VK_NULL_HANDLE,e.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},{sampler,VK_NULL_HANDLE,VK_IMAGE_LAYOUT_UNDEFINED},{VK_NULL_HANDLE,withSelfIllumMask?maskEntries[n].view:VK_NULL_HANDLE,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},{VK_NULL_HANDLE,secondEntries[n].view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};VkWriteDescriptorSet writes[6]{};auto initWrite=[&](uint32_t index,uint32_t binding,uint32_t imageIndex,VkDescriptorType type){auto&w=writes[index];w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.dstSet=e.set;w.dstBinding=binding;w.descriptorCount=1;w.descriptorType=type;w.pImageInfo=&images[imageIndex];};initWrite(0,0,0,VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);initWrite(1,1,1,VK_DESCRIPTOR_TYPE_SAMPLER);initWrite(2,3,3,VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);uint32_t writeCount=3;if(withSelfIllumMask){initWrite(3,2,2,VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);writeCount=4;}if(independentSamplers){initWrite(writeCount++,4,1,VK_DESCRIPTOR_TYPE_SAMPLER);initWrite(writeCount++,5,1,VK_DESCRIPTOR_TYPE_SAMPLER);}load<PFN_vkUpdateDescriptorSets>(library,"vkUpdateDescriptorSets")(device,writeCount,writes,0,nullptr);
  }
 }
  void upload(VkCommandBuffer command,VkPipelineStageFlags shaderStages=VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT){if(!shaderStages)throw std::runtime_error("material upload shader stages missing");auto barrier=load<PFN_vkCmdPipelineBarrier>(library,"vkCmdPipelineBarrier");for(auto*collection:{&entries,&maskEntries,&secondEntries})for(auto&e:*collection){VkImageMemoryBarrier b{};b.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;b.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;b.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;b.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.image=e.image;b.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,e.mipLevels,0,1};barrier(command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&b);VkBufferImageCopy region{};region.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};region.imageExtent={e.width,e.height,1};load<PFN_vkCmdCopyBufferToImage>(library,"vkCmdCopyBufferToImage")(command,e.staging,e.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,e.uploadRegions.empty()?1u:uint32_t(e.uploadRegions.size()),e.uploadRegions.empty()?&region:e.uploadRegions.data());b.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;b.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;b.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;b.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;barrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,shaderStages,0,0,nullptr,0,nullptr,1,&b);}}
};
