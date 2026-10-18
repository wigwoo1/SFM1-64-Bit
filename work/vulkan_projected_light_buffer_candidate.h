#pragma once
// Caller retains this owner until all GPU submissions using its descriptor finish.
struct VulkanProjectedLightBufferCandidate {
 HMODULE library;VkDevice device;VkBuffer buffer=VK_NULL_HANDLE;VkDeviceMemory memory=VK_NULL_HANDLE;VkDeviceSize bytes=0;
 VulkanProjectedLightBufferCandidate(HMODULE l,VkDevice d):library(l),device(d){}
 VulkanProjectedLightBufferCandidate(const VulkanProjectedLightBufferCandidate&)=delete;
 VulkanProjectedLightBufferCandidate&operator=(const VulkanProjectedLightBufferCandidate&)=delete;
 ~VulkanProjectedLightBufferCandidate(){if(buffer)load<PFN_vkDestroyBuffer>(library,"vkDestroyBuffer")(device,buffer,nullptr);if(memory)load<PFN_vkFreeMemory>(library,"vkFreeMemory")(device,memory,nullptr);}
 static void check(VkResult r){if(r!=VK_SUCCESS)throw std::runtime_error("projected light buffer Vulkan result "+std::to_string(r));}
 void create(VkPhysicalDevice physical,const void*data,size_t size,VkBufferUsageFlags extraUsage=0){
  if(buffer||memory)throw std::runtime_error("light buffer already allocated");
  if(!data||!size||size%112)throw std::runtime_error("light buffer record shape");
  VkPhysicalDeviceProperties caps{};load<PFN_vkGetPhysicalDeviceProperties>(library,"vkGetPhysicalDeviceProperties")(physical,&caps);
  if(size>caps.limits.maxStorageBufferRange)throw std::runtime_error("light buffer device range exceeded");
  VkBufferCreateInfo info{};info.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;info.size=size;info.usage=VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|extraUsage;info.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
  check(load<PFN_vkCreateBuffer>(library,"vkCreateBuffer")(device,&info,nullptr,&buffer));
  VkMemoryRequirements requirement{};load<PFN_vkGetBufferMemoryRequirements>(library,"vkGetBufferMemoryRequirements")(device,buffer,&requirement);
  VkPhysicalDeviceMemoryProperties properties{};load<PFN_vkGetPhysicalDeviceMemoryProperties>(library,"vkGetPhysicalDeviceMemoryProperties")(physical,&properties);
  uint32_t type=UINT32_MAX;VkMemoryPropertyFlags flags=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  for(uint32_t i=0;i<properties.memoryTypeCount;++i)if((requirement.memoryTypeBits&(1u<<i))&&(properties.memoryTypes[i].propertyFlags&flags)==flags){type=i;break;}
  if(type==UINT32_MAX)throw std::runtime_error("light buffer coherent memory unavailable");
  VkMemoryAllocateInfo allocation{};allocation.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;allocation.allocationSize=requirement.size;allocation.memoryTypeIndex=type;
  check(load<PFN_vkAllocateMemory>(library,"vkAllocateMemory")(device,&allocation,nullptr,&memory));check(load<PFN_vkBindBufferMemory>(library,"vkBindBufferMemory")(device,buffer,memory,0));
  void*mapped=nullptr;check(load<PFN_vkMapMemory>(library,"vkMapMemory")(device,memory,0,size,0,&mapped));std::memcpy(mapped,data,size);load<PFN_vkUnmapMemory>(library,"vkUnmapMemory")(device,memory);bytes=size;
 }
 void replaceAfterCompletedSubmission(const void*data,size_t size,bool submissionPending){
  if(submissionPending)throw std::runtime_error("light buffer update while submission pending");
  if(!buffer||!memory||!bytes||!data||size!=bytes)throw std::runtime_error("light buffer update shape");
  void*mapped=nullptr;check(load<PFN_vkMapMemory>(library,"vkMapMemory")(device,memory,0,bytes,0,&mapped));std::memcpy(mapped,data,size);load<PFN_vkUnmapMemory>(library,"vkUnmapMemory")(device,memory);
 }
 VkDescriptorBufferInfo descriptor()const{if(!buffer||!bytes)throw std::runtime_error("light buffer unprepared");return {buffer,0,bytes};}
};

inline void testVulkanProjectedLightBufferCandidate(HMODULE library,VkDevice device,VkPhysicalDevice physical){
 std::array<uint8_t,336> fixture{};for(size_t i=0;i<fixture.size();++i)fixture[i]=uint8_t(i*37+11);
 VulkanProjectedLightBufferCandidate owner(library,device);bool malformed=false;try{owner.create(physical,fixture.data(),335);}catch(const std::runtime_error&){malformed=true;}if(!malformed||owner.buffer||owner.memory)throw std::runtime_error("malformed light buffer allocated");
 owner.create(physical,fixture.data(),fixture.size());auto descriptor=owner.descriptor();if(descriptor.buffer!=owner.buffer||descriptor.offset||descriptor.range!=fixture.size())throw std::runtime_error("light descriptor range");
 void*mapped=nullptr;owner.check(load<PFN_vkMapMemory>(library,"vkMapMemory")(device,owner.memory,0,fixture.size(),0,&mapped));bool equal=std::memcmp(mapped,fixture.data(),fixture.size())==0;load<PFN_vkUnmapMemory>(library,"vkUnmapMemory")(device,owner.memory);if(!equal)throw std::runtime_error("light mapped upload bytes");
 bool repeat=false;try{owner.create(physical,fixture.data(),fixture.size());}catch(const std::runtime_error&){repeat=true;}if(!repeat)throw std::runtime_error("light buffer overwritten");
 std::cout<<"projected_light_buffer_device_test=passed records=3 bytes=336 host_mapped_bytes_equal=1 shader_read=unverified\n";
}
