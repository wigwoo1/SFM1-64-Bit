#pragma once
// Borrows image views, sampler and immutable light buffer. Their owners must
// outlive this pool and every submission using its sets.
struct VulkanProjectedLightDescriptorsCandidate {
 HMODULE library;VkDevice device;VkDescriptorSetLayout layout{};VkDescriptorPool pool{};
 bool withShadow=false;std::vector<VkDescriptorSet>sets;size_t materials=0,lights=0;
 VulkanProjectedLightDescriptorsCandidate(HMODULE l,VkDevice d):library(l),device(d){}
 VulkanProjectedLightDescriptorsCandidate(const VulkanProjectedLightDescriptorsCandidate&)=delete;
 ~VulkanProjectedLightDescriptorsCandidate(){if(pool)load<PFN_vkDestroyDescriptorPool>(library,"vkDestroyDescriptorPool")(device,pool,nullptr);if(layout)load<PFN_vkDestroyDescriptorSetLayout>(library,"vkDestroyDescriptorSetLayout")(device,layout,nullptr);}
 void create(const std::vector<VkImageView>&baseViews,const std::vector<VkImageView>&goboViews,VkSampler sampler,VkDescriptorBufferInfo buffer,VkImageView shadowView=VK_NULL_HANDLE,VkSampler shadowSampler=VK_NULL_HANDLE){
  if(layout||pool)throw std::runtime_error("light descriptors already created");
  if(bool(shadowView)!=bool(shadowSampler))throw std::runtime_error("shadow descriptor pair missing");withShadow=bool(shadowView);
  if(baseViews.empty()||goboViews.empty()||!sampler||!buffer.buffer||buffer.offset||buffer.range!=goboViews.size()*112ull)throw std::runtime_error("light descriptor coverage");
  if(goboViews.size()>UINT32_MAX/3||baseViews.size()>UINT32_MAX/3/goboViews.size())throw std::runtime_error("light descriptor count overflow");
  for(auto view:baseViews)if(!view)throw std::runtime_error("missing light base view");for(auto view:goboViews)if(!view)throw std::runtime_error("missing gobo view");
  auto check=VulkanProjectedLightBufferCandidate::check;materials=baseViews.size();lights=goboViews.size();uint32_t count=uint32_t(materials*lights);
  VkDescriptorSetLayoutBinding bindings[6]{{0,VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},{1,VK_DESCRIPTOR_TYPE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},{3,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},{4,VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},{6,VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},{7,VK_DESCRIPTOR_TYPE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}};
  VkDescriptorSetLayoutCreateInfo li{};li.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;li.bindingCount=withShadow?6:4;li.pBindings=bindings;check(load<PFN_vkCreateDescriptorSetLayout>(library,"vkCreateDescriptorSetLayout")(device,&li,nullptr,&layout));
  VkDescriptorPoolSize sizes[3]{{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,count*(withShadow?3u:2u)},{VK_DESCRIPTOR_TYPE_SAMPLER,count*(withShadow?2u:1u)},{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,count}};VkDescriptorPoolCreateInfo pi{};pi.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;pi.maxSets=count;pi.poolSizeCount=3;pi.pPoolSizes=sizes;check(load<PFN_vkCreateDescriptorPool>(library,"vkCreateDescriptorPool")(device,&pi,nullptr,&pool));
  std::vector<VkDescriptorSetLayout>layouts(count,layout);sets.resize(count);VkDescriptorSetAllocateInfo ai{};ai.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;ai.descriptorPool=pool;ai.descriptorSetCount=count;ai.pSetLayouts=layouts.data();check(load<PFN_vkAllocateDescriptorSets>(library,"vkAllocateDescriptorSets")(device,&ai,sets.data()));
  for(size_t m=0;m<materials;++m)for(size_t l=0;l<lights;++l){VkDescriptorImageInfo images[3]{{VK_NULL_HANDLE,baseViews[m],VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},{sampler,VK_NULL_HANDLE,VK_IMAGE_LAYOUT_UNDEFINED},{VK_NULL_HANDLE,goboViews[l],VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};VkWriteDescriptorSet writes[4]{};for(unsigned i=0;i<4;++i){writes[i].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;writes[i].dstSet=sets[m*lights+l];writes[i].descriptorCount=1;writes[i].dstBinding=i<2?i:i==2?3:4;writes[i].descriptorType=i==1?VK_DESCRIPTOR_TYPE_SAMPLER:i==2?VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;if(i==2)writes[i].pBufferInfo=&buffer;else writes[i].pImageInfo=&images[i==3?2:i];}load<PFN_vkUpdateDescriptorSets>(library,"vkUpdateDescriptorSets")(device,4,writes,0,nullptr);}
  if(withShadow)replaceShadowAfterCompletedSubmission(shadowView,shadowSampler,false);
 }
 void replaceShadowAfterCompletedSubmission(VkImageView view,VkSampler sampler,bool pending){
  if(pending||!withShadow||!view||!sampler)throw std::runtime_error("shadow descriptor update state");
  VkDescriptorImageInfo images[2]{{VK_NULL_HANDLE,view,VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL},{sampler,VK_NULL_HANDLE,VK_IMAGE_LAYOUT_UNDEFINED}};
  for(auto set:sets){VkWriteDescriptorSet writes[2]{};for(unsigned n=0;n<2;++n){writes[n].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;writes[n].dstSet=set;writes[n].dstBinding=6+n;writes[n].descriptorCount=1;writes[n].descriptorType=n?VK_DESCRIPTOR_TYPE_SAMPLER:VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;writes[n].pImageInfo=&images[n];}load<PFN_vkUpdateDescriptorSets>(library,"vkUpdateDescriptorSets")(device,2,writes,0,nullptr);}
 }
 VkDescriptorSet at(size_t material,size_t light)const{if(material>=materials||light>=lights)throw std::runtime_error("light descriptor address");return sets.at(material*lights+light);}
};
