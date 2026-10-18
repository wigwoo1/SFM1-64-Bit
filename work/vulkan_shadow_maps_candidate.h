#pragma once
#include <set>
#include "source_shadow_settings_candidate.h"
// Caller must finish all GPU uses before retiring this depth-image owner.
struct VulkanShadowMapsCandidate {
 HMODULE library;VkDevice device;VkImage image{};VkDeviceMemory memory{};
 VkImageView arrayView{};std::vector<VkImageView>layerViews;VkDeviceSize allocationBytes=0;
 uint32_t resolution=0,layers=0;VkFormat format=VK_FORMAT_D32_SFLOAT;
 VulkanShadowMapsCandidate(HMODULE l,VkDevice d):library(l),device(d){}
 VulkanShadowMapsCandidate(const VulkanShadowMapsCandidate&)=delete;
 VulkanShadowMapsCandidate&operator=(const VulkanShadowMapsCandidate&)=delete;
 ~VulkanShadowMapsCandidate(){for(auto view:layerViews)if(view)load<PFN_vkDestroyImageView>(library,"vkDestroyImageView")(device,view,nullptr);if(arrayView)load<PFN_vkDestroyImageView>(library,"vkDestroyImageView")(device,arrayView,nullptr);if(image)load<PFN_vkDestroyImage>(library,"vkDestroyImage")(device,image,nullptr);if(memory)load<PFN_vkFreeMemory>(library,"vkFreeMemory")(device,memory,nullptr);}
 void create(VkPhysicalDevice physical,uint32_t size,uint32_t count){
  if(image||memory||arrayView||!layerViews.empty())throw std::runtime_error("shadow maps already allocated");
  if(!size||!count)throw std::runtime_error("shadow map dimensions empty");
  VkPhysicalDeviceProperties caps{};load<PFN_vkGetPhysicalDeviceProperties>(library,"vkGetPhysicalDeviceProperties")(physical,&caps);
  if(size>caps.limits.maxImageDimension2D||count>caps.limits.maxImageArrayLayers||size>caps.limits.maxFramebufferWidth||size>caps.limits.maxFramebufferHeight)throw std::runtime_error("shadow map device dimensions exceeded");
  VkFormatProperties features{};load<PFN_vkGetPhysicalDeviceFormatProperties>(library,"vkGetPhysicalDeviceFormatProperties")(physical,format,&features);
  const VkFormatFeatureFlags required=VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
  if((features.optimalTilingFeatures&required)!=required)throw std::runtime_error("shadow map depth sampling unsupported");
  auto check=VulkanProjectedLightBufferCandidate::check;
  const VkImageUsageFlags usage=VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  VkImageFormatProperties limits{};check(load<PFN_vkGetPhysicalDeviceImageFormatProperties>(library,"vkGetPhysicalDeviceImageFormatProperties")(physical,format,VK_IMAGE_TYPE_2D,VK_IMAGE_TILING_OPTIMAL,usage,0,&limits));
  if(size>limits.maxExtent.width||size>limits.maxExtent.height||count>limits.maxArrayLayers)throw std::runtime_error("shadow map format dimensions exceeded");
  VkImageCreateInfo info{};info.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;info.imageType=VK_IMAGE_TYPE_2D;info.format=format;info.extent={size,size,1};info.mipLevels=1;info.arrayLayers=count;info.samples=VK_SAMPLE_COUNT_1_BIT;info.tiling=VK_IMAGE_TILING_OPTIMAL;info.usage=usage;info.sharingMode=VK_SHARING_MODE_EXCLUSIVE;info.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
  check(load<PFN_vkCreateImage>(library,"vkCreateImage")(device,&info,nullptr,&image));
  VkMemoryRequirements requirement{};load<PFN_vkGetImageMemoryRequirements>(library,"vkGetImageMemoryRequirements")(device,image,&requirement);
  if(requirement.size>limits.maxResourceSize)throw std::runtime_error("shadow map resource size exceeded");
  VkPhysicalDeviceMemoryProperties properties{};load<PFN_vkGetPhysicalDeviceMemoryProperties>(library,"vkGetPhysicalDeviceMemoryProperties")(physical,&properties);
  uint32_t type=UINT32_MAX;for(uint32_t n=0;n<properties.memoryTypeCount;++n)if((requirement.memoryTypeBits&(uint32_t(1)<<n))&&(properties.memoryTypes[n].propertyFlags&VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)){type=n;break;}
  if(type==UINT32_MAX)throw std::runtime_error("shadow map device memory missing");
  VkMemoryAllocateInfo allocation{};allocation.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;allocation.allocationSize=requirement.size;allocation.memoryTypeIndex=type;
  check(load<PFN_vkAllocateMemory>(library,"vkAllocateMemory")(device,&allocation,nullptr,&memory));check(load<PFN_vkBindImageMemory>(library,"vkBindImageMemory")(device,image,memory,0));allocationBytes=requirement.size;
  VkImageViewCreateInfo view{};view.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;view.image=image;view.format=format;view.viewType=VK_IMAGE_VIEW_TYPE_2D_ARRAY;view.subresourceRange={VK_IMAGE_ASPECT_DEPTH_BIT,0,1,0,count};
  check(load<PFN_vkCreateImageView>(library,"vkCreateImageView")(device,&view,nullptr,&arrayView));
  layerViews.resize(count);view.viewType=VK_IMAGE_VIEW_TYPE_2D;view.subresourceRange.layerCount=1;for(uint32_t n=0;n<count;++n){view.subresourceRange.baseArrayLayer=n;check(load<PFN_vkCreateImageView>(library,"vkCreateImageView")(device,&view,nullptr,&layerViews[n]));}
  resolution=size;layers=count;
 }
};
struct VulkanShadowComparisonSamplerCandidate {
 HMODULE library;VkDevice device;VkSampler sampler{};
 VulkanShadowComparisonSamplerCandidate(HMODULE l,VkDevice d):library(l),device(d){VkSamplerCreateInfo info{};info.sType=VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;info.magFilter=info.minFilter=VK_FILTER_NEAREST;info.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;info.addressModeU=info.addressModeV=info.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;info.maxAnisotropy=1;info.compareEnable=VK_TRUE;info.compareOp=VK_COMPARE_OP_LESS_OR_EQUAL;VulkanProjectedLightBufferCandidate::check(load<PFN_vkCreateSampler>(library,"vkCreateSampler")(device,&info,nullptr,&sampler));}
 VulkanShadowComparisonSamplerCandidate(const VulkanShadowComparisonSamplerCandidate&)=delete;
 ~VulkanShadowComparisonSamplerCandidate(){if(sampler)load<PFN_vkDestroySampler>(library,"vkDestroySampler")(device,sampler,nullptr);}
};
// Diagnostic either clears fresh UNDEFINED layers or verifies rasterized depth-read-only layers.
inline void testVulkanShadowDepthLayersCandidate(HMODULE library,VkDevice device,VkPhysicalDevice physical,uint32_t family,VulkanShadowMapsCandidate&owner,bool initializeDepth=true,std::vector<float>*sceneDepth=nullptr,const std::vector<std::array<float,2>>*biasFactors=nullptr){
 if(!owner.resolution||(!sceneDepth&&(owner.resolution!=64||owner.layers!=256))||owner.format!=VK_FORMAT_D32_SFLOAT)throw std::runtime_error("shadow depth fixture shape");
 auto check=VulkanProjectedLightBufferCandidate::check;
 const size_t pixels=size_t(owner.resolution)*owner.resolution,bytes=pixels*owner.layers*4,padded=(bytes+111)/112*112;
 std::vector<uint8_t>zero(padded);VulkanProjectedLightBufferCandidate output(library,device);output.create(physical,zero.data(),padded,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
 VkCommandPool pool{};VkQueue queue{};bool submitted=false;load<PFN_vkGetDeviceQueue>(library,"vkGetDeviceQueue")(device,family,0,&queue);
 auto cleanup=[&]{if(submitted)load<PFN_vkQueueWaitIdle>(library,"vkQueueWaitIdle")(queue);if(pool)load<PFN_vkDestroyCommandPool>(library,"vkDestroyCommandPool")(device,pool,nullptr);};
 try{
  VkCommandPoolCreateInfo pi{};pi.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;pi.queueFamilyIndex=family;check(load<PFN_vkCreateCommandPool>(library,"vkCreateCommandPool")(device,&pi,nullptr,&pool));
  VkCommandBuffer command{};VkCommandBufferAllocateInfo ai{};ai.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;ai.commandPool=pool;ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ai.commandBufferCount=1;check(load<PFN_vkAllocateCommandBuffers>(library,"vkAllocateCommandBuffers")(device,&ai,&command));
  VkCommandBufferBeginInfo bi{};bi.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;check(load<PFN_vkBeginCommandBuffer>(library,"vkBeginCommandBuffer")(command,&bi));
  auto barrier=load<PFN_vkCmdPipelineBarrier>(library,"vkCmdPipelineBarrier");VkImageMemoryBarrier image{};image.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;image.image=owner.image;image.srcQueueFamilyIndex=image.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;image.subresourceRange={VK_IMAGE_ASPECT_DEPTH_BIT,0,1,0,owner.layers};image.oldLayout=initializeDepth?VK_IMAGE_LAYOUT_UNDEFINED:VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;image.newLayout=initializeDepth?VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;image.srcAccessMask=initializeDepth?0:VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;image.dstAccessMask=initializeDepth?VK_ACCESS_TRANSFER_WRITE_BIT:VK_ACCESS_TRANSFER_READ_BIT;
  barrier(command,initializeDepth?VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT:(VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT),VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&image);
  std::vector<uint32_t>expected(owner.layers);for(uint32_t layer=0;layer<owner.layers;++layer){float depth=float(layer+1)/float(owner.layers+1);if(biasFactors)depth+=biasFactors->at(layer)[0];std::memcpy(&expected[layer],&depth,4);VkClearDepthStencilValue value{depth,0};VkImageSubresourceRange range{VK_IMAGE_ASPECT_DEPTH_BIT,0,1,layer,1};if(initializeDepth)load<PFN_vkCmdClearDepthStencilImage>(library,"vkCmdClearDepthStencilImage")(command,owner.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&value,1,&range);}
  if(initializeDepth){image.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;image.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;image.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;image.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;barrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&image);}
  std::vector<VkBufferImageCopy>regions(owner.layers);for(uint32_t layer=0;layer<owner.layers;++layer){regions[layer].bufferOffset=VkDeviceSize(layer)*pixels*4;regions[layer].imageSubresource={VK_IMAGE_ASPECT_DEPTH_BIT,0,layer,1};regions[layer].imageExtent={owner.resolution,owner.resolution,1};}
  load<PFN_vkCmdCopyImageToBuffer>(library,"vkCmdCopyImageToBuffer")(command,owner.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,output.buffer,uint32_t(regions.size()),regions.data());
  image.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;image.newLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;image.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;image.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;barrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,0,nullptr,1,&image);
  VkMemoryBarrier host{};host.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;barrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&host,0,nullptr,0,nullptr);
  check(load<PFN_vkEndCommandBuffer>(library,"vkEndCommandBuffer")(command));VkSubmitInfo submit{};submit.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;submit.commandBufferCount=1;submit.pCommandBuffers=&command;check(load<PFN_vkQueueSubmit>(library,"vkQueueSubmit")(queue,1,&submit,VK_NULL_HANDLE));submitted=true;check(load<PFN_vkQueueWaitIdle>(library,"vkQueueWaitIdle")(queue));submitted=false;
  void*mapped=nullptr;check(load<PFN_vkMapMemory>(library,"vkMapMemory")(device,output.memory,0,padded,0,&mapped));auto words=static_cast<const uint32_t*>(mapped);bool equal=true;size_t nonclear=0;if(sceneDepth){sceneDepth->resize(pixels*owner.layers);std::memcpy(sceneDepth->data(),mapped,bytes);for(float depth:*sceneDepth){if(!std::isfinite(depth)||depth<0||depth>1)equal=false;if(depth<1)++nonclear;}}else for(uint32_t layer=0;layer<owner.layers;++layer)for(size_t pixel=0;pixel<pixels;++pixel)if(words[size_t(layer)*pixels+pixel]!=expected[layer])equal=false;load<PFN_vkUnmapMemory>(library,"vkUnmapMemory")(device,output.memory);if(!equal)throw std::runtime_error("shadow depth layer isolation mismatch");
  cleanup();if(sceneDepth){std::cout<<"shadow_scene_depth_readback=passed layers="<<owner.layers<<" nonclear_pixels="<<nonclear<<" bytes="<<bytes<<" source_parity=unverified\n";return;}std::cout<<"shadow_depth_layer_readback=passed layers=256 pixels="<<pixels*owner.layers<<" bytes="<<bytes<<" byte_exact=1 final_layout=depth_read_only caster_rendering=unfinished\n";
 }catch(...){cleanup();throw;}
}
#include "vulkan_shadow_depth_shader_candidate.h"
#include "vulkan_shadow_depth_raster_candidate.h"
inline void testVulkanShadowMapsCandidate(HMODULE library,VkDevice device,VkPhysicalDevice physical,uint32_t family){
 testSourceShadowSettingsCandidate();
 VulkanShadowMapsCandidate owner(library,device);bool rejected=false;try{owner.create(physical,0,256);}catch(const std::runtime_error&){rejected=true;}if(!rejected||owner.image)throw std::runtime_error("empty shadow dimensions accepted");
 VkPhysicalDeviceProperties caps{};load<PFN_vkGetPhysicalDeviceProperties>(library,"vkGetPhysicalDeviceProperties")(physical,&caps);
 rejected=false;try{owner.create(physical,caps.limits.maxImageDimension2D+1,256);}catch(const std::runtime_error&){rejected=true;}if(!rejected||owner.image)throw std::runtime_error("oversize shadow dimensions accepted");
 owner.create(physical,64,256);if(owner.layerViews.size()!=256||!owner.arrayView||!owner.allocationBytes)throw std::runtime_error("shadow map layer coverage");std::set<VkImageView>unique(owner.layerViews.begin(),owner.layerViews.end());if(unique.size()!=256||unique.count(VK_NULL_HANDLE))throw std::runtime_error("shadow map view coverage");
 rejected=false;try{owner.create(physical,64,256);}catch(const std::runtime_error&){rejected=true;}if(!rejected)throw std::runtime_error("shadow map second allocation accepted");
 testVulkanShadowDepthRasterCandidate(library,device,family,owner);
 testVulkanShadowDepthLayersCandidate(library,device,physical,family,owner,false);
 testVulkanShadowDepthShaderCandidate(library,device,physical,family,owner);
 std::cout<<"shadow_map_device_allocation=passed layers=256 resolution=64 allocation_bytes="<<owner.allocationBytes<<" depth_rendering=unfinished\n";
}

inline void testVulkanShadowConstantBiasCandidate(HMODULE library,VkDevice device,VkPhysicalDevice physical,uint32_t family){
 std::vector<std::array<float,2>>bias(256);for(size_t layer=0;layer<bias.size();++layer)bias[layer]={float(int(layer%3)-1)/65536.0f,0};
 VulkanShadowMapsCandidate owner(library,device);owner.create(physical,64,256);testVulkanShadowDepthRasterCandidate(library,device,family,owner,{},&bias);testVulkanShadowDepthLayersCandidate(library,device,physical,family,owner,false,nullptr,&bias);
 std::vector<std::array<float,2>> slopeBias(256);for(size_t layer=0;layer<256;++layer)slopeBias[layer]={0,float(int(layer%3)-1)};
 VulkanShadowMapsCandidate sloped(library,device);sloped.create(physical,64,256);
 testVulkanShadowDepthRasterCandidate(library,device,family,sloped,{},&slopeBias,1.0f/1024.0f);
 std::vector<float> depths;testVulkanShadowDepthLayersCandidate(library,device,physical,family,sloped,false,&depths);
 float maximumError=0;for(size_t layer=0;layer<256;++layer)for(size_t y=0;y<64;++y)for(size_t x=0;x<64;++x){float expected=float(layer+1)/257.0f+(float(x)+0.5f)/65536.0f+slopeBias[layer][1]/65536.0f;float error=std::abs(depths[layer*4096+y*64+x]-expected);maximumError=std::max(maximumError,error);if(error>0.00000024f)throw std::runtime_error("shadow slope bias GPU mismatch");}
 std::cout<<"source_shadow_slope_bias_gpu=passed layers=256 pixels=1048576 slope_factors=negative,zero,positive maximum_absolute_error="<<maximumError<<" saved_scene_bias=disabled\n";
 std::cout<<"source_shadow_constant_bias_gpu=passed layers=256 pixels=1048576 offsets=negative,zero,positive representation=exact_force_unorm slope=untested saved_scene_bias=disabled\n";
}
