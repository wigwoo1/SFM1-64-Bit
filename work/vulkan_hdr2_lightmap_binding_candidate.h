#pragma once
#include "vulkan_hdr2_lightmap_images_candidate.h"
#include "vulkan_prepared_scene_candidate.h"
#include <tuple>

// References existing geometry; only the ordered draw lists are split/copied.
struct VulkanHdr2SceneDrawViewCandidate {
    const std::vector<SceneGpuVertexCandidate>&vertices;
    const std::vector<uint32_t>&indices;
    const std::vector<SceneGpuMaterialAddressCandidate>&materials;
    std::vector<SceneGpuDrawCandidate>opaque,translucent;
    VulkanHdr2SceneDrawViewCandidate(const SceneGpuUploadCandidate&u,const SceneHdr2PlanCandidate&p):vertices(u.vertices),indices(u.indices),materials(u.materials){
        if(p.lightUvs.size()!=u.vertices.size())throw std::runtime_error("HDR2 draw view vertex coverage");
        for(const auto&b:p.opaque)opaque.push_back(b.draw);for(const auto&b:p.translucent)translucent.push_back(b.draw);
    }
};
// Confined to caller device/queue. Keep this owner and compatible pipeline/layout
// alive through EVERY sampled draw fence, including draws after upload completion.
class VulkanHdr2LightmapBindingCandidate:public std::enable_shared_from_this<VulkanHdr2LightmapBindingCandidate> {
    HMODULE library_;VkDevice device_;
    std::shared_ptr<VulkanHdr2LightmapImagesCandidate>images_;
    VkDescriptorPool pool_=VK_NULL_HANDLE;VkSampler sampler_=VK_NULL_HANDLE;
    VkBuffer uvs_=VK_NULL_HANDLE;VkDeviceMemory uvMemory_=VK_NULL_HANDLE;
    std::vector<VkDescriptorSet>sets_;std::array<float,4>push_{};
    using Key=std::tuple<size_t,size_t,uint32_t,uint32_t,uint32_t,uint32_t>;
    std::map<Key,size_t>resources_;
    static Key key(const SceneGpuDrawCandidate&d){return {d.model,d.range,d.firstIndex,d.indexCount,d.materialIndex,d.materialSlot};}
    VulkanHdr2LightmapBindingCandidate(HMODULE l,VkDevice d,std::shared_ptr<VulkanHdr2LightmapImagesCandidate>i):library_(l),device_(d),images_(std::move(i)){}
    size_t resource(const SceneGpuDrawCandidate&d)const{auto found=resources_.find(key(d));if(found==resources_.end())throw std::runtime_error("HDR2 draw absent from immutable plan");return found->second;}
public:
    ~VulkanHdr2LightmapBindingCandidate(){if(pool_)load<PFN_vkDestroyDescriptorPool>(library_,"vkDestroyDescriptorPool")(device_,pool_,nullptr);if(sampler_)load<PFN_vkDestroySampler>(library_,"vkDestroySampler")(device_,sampler_,nullptr);if(uvs_)load<PFN_vkDestroyBuffer>(library_,"vkDestroyBuffer")(device_,uvs_,nullptr);if(uvMemory_)load<PFN_vkFreeMemory>(library_,"vkFreeMemory")(device_,uvMemory_,nullptr);}
    VulkanHdr2LightmapBindingCandidate(const VulkanHdr2LightmapBindingCandidate&)=delete;
    static std::shared_ptr<VulkanHdr2LightmapBindingCandidate> prepare(HMODULE library,VkDevice device,VkPhysicalDevice physical,
        std::shared_ptr<VulkanHdr2LightmapImagesCandidate>images,VkDescriptorSetLayout lightLayout,const std::function<void(const char*)>&hook={},VkFilter filter=VK_FILTER_NEAREST){
        if(!library||!device||!physical||!images||!lightLayout||images->entries().empty())throw std::runtime_error("HDR2 sampled binding inputs");
        if(filter!=VK_FILTER_NEAREST&&filter!=VK_FILTER_LINEAR)throw std::runtime_error("HDR2 unsupported sampler filter");
        if(filter==VK_FILTER_LINEAR){VkFormatProperties properties{};load<PFN_vkGetPhysicalDeviceFormatProperties>(library,"vkGetPhysicalDeviceFormatProperties")(physical,images->format(),&properties);if(!(properties.optimalTilingFeatures&VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))throw std::runtime_error("HDR2 linear sampling unsupported by format");}
        const auto&plan=*images->plan();if(plan.faces.size()!=images->entries().size()||plan.faces.size()>UINT32_MAX||plan.lightUvs.empty()||plan.lightUvs.size()>SIZE_MAX/sizeof(plan.lightUvs[0]))throw std::runtime_error("HDR2 sampled binding coverage");
        auto stage=[&](const char*s){if(hook)hook(s);};auto next=std::shared_ptr<VulkanHdr2LightmapBindingCandidate>(new VulkanHdr2LightmapBindingCandidate(library,device,std::move(images)));
        bool constant=false;for(const auto&c:plan.modulation.constants)if(!c.vertex&&c.index==12){if(constant)throw std::runtime_error("HDR2 ambiguous light modulation");next->push_={c.value[0],c.value[1],c.value[2],plan.policy.exposure};constant=true;}if(!constant)throw std::runtime_error("HDR2 light modulation missing");
        for(const auto*list:{&plan.opaque,&plan.translucent})for(const auto&b:*list){if(b.faceResource!=SIZE_MAX&&b.faceResource>=plan.faces.size())throw std::runtime_error("HDR2 face resource out of range");if(!next->resources_.emplace(key(b.draw),b.faceResource).second)throw std::runtime_error("HDR2 duplicate draw identity");}
        VkPhysicalDeviceMemoryProperties properties{};load<PFN_vkGetPhysicalDeviceMemoryProperties>(library,"vkGetPhysicalDeviceMemoryProperties")(physical,&properties);
        VkBufferCreateInfo bi{};bi.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;bi.size=plan.lightUvs.size()*sizeof(plan.lightUvs[0]);bi.usage=VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;VulkanMaterialImagesCandidate::check(load<PFN_vkCreateBuffer>(library,"vkCreateBuffer")(device,&bi,nullptr,&next->uvs_));stage("uv-buffer");VkMemoryRequirements req{};load<PFN_vkGetBufferMemoryRequirements>(library,"vkGetBufferMemoryRequirements")(device,next->uvs_,&req);uint32_t type=UINT32_MAX;for(uint32_t n=0;n<properties.memoryTypeCount;++n)if((req.memoryTypeBits&(1u<<n))&&(properties.memoryTypes[n].propertyFlags&(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))==(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){type=n;break;}if(type==UINT32_MAX)throw std::runtime_error("HDR2 UV host memory");VkMemoryAllocateInfo mi{};mi.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;mi.allocationSize=req.size;mi.memoryTypeIndex=type;VulkanMaterialImagesCandidate::check(load<PFN_vkAllocateMemory>(library,"vkAllocateMemory")(device,&mi,nullptr,&next->uvMemory_));stage("uv-memory");VulkanMaterialImagesCandidate::check(load<PFN_vkBindBufferMemory>(library,"vkBindBufferMemory")(device,next->uvs_,next->uvMemory_,0));void*m=nullptr;VulkanMaterialImagesCandidate::check(load<PFN_vkMapMemory>(library,"vkMapMemory")(device,next->uvMemory_,0,bi.size,0,&m));std::memcpy(m,plan.lightUvs.data(),size_t(bi.size));load<PFN_vkUnmapMemory>(library,"vkUnmapMemory")(device,next->uvMemory_);stage("uv-filled");
        VkSamplerCreateInfo si{};si.sType=VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;si.minFilter=si.magFilter=filter;si.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;si.addressModeU=si.addressModeV=si.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;VulkanMaterialImagesCandidate::check(load<PFN_vkCreateSampler>(library,"vkCreateSampler")(device,&si,nullptr,&next->sampler_));stage("sampler");
        const auto count=uint32_t(plan.faces.size());VkDescriptorPoolSize sizes[2]{{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,count},{VK_DESCRIPTOR_TYPE_SAMPLER,count}};VkDescriptorPoolCreateInfo pi{};pi.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;pi.maxSets=count;pi.poolSizeCount=2;pi.pPoolSizes=sizes;VulkanMaterialImagesCandidate::check(load<PFN_vkCreateDescriptorPool>(library,"vkCreateDescriptorPool")(device,&pi,nullptr,&next->pool_));stage("pool");next->sets_.resize(count);
        for(uint32_t n=0;n<count;++n){VkDescriptorSetAllocateInfo ai{};ai.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;ai.descriptorPool=next->pool_;ai.descriptorSetCount=1;ai.pSetLayouts=&lightLayout;VulkanMaterialImagesCandidate::check(load<PFN_vkAllocateDescriptorSets>(library,"vkAllocateDescriptorSets")(device,&ai,&next->sets_[n]));stage("set");VkDescriptorImageInfo info[2]{{VK_NULL_HANDLE,next->images_->entries()[n].view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},{next->sampler_,VK_NULL_HANDLE,VK_IMAGE_LAYOUT_UNDEFINED}};VkWriteDescriptorSet writes[2]{};for(uint32_t k=0;k<2;++k){writes[k].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;writes[k].dstSet=next->sets_[n];writes[k].dstBinding=k;writes[k].descriptorType=k?VK_DESCRIPTOR_TYPE_SAMPLER:VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;writes[k].descriptorCount=1;writes[k].pImageInfo=&info[k];}load<PFN_vkUpdateDescriptorSets>(library,"vkUpdateDescriptorSets")(device,2,writes,0,nullptr);stage("set-written");}
        stage("binding-ready");return next;
    }
    // Pipeline and shared resources.layout must expose compatible set0/set1 and
    // vertex push0..63, fragment96..159. Does not replace existing callbacks.
    void configure(VulkanPreparedSceneResourcesCandidate&resources,VkPipeline hdrPipeline,VkCommandBuffer command){
        if(!hdrPipeline||!command||!resources.layout||resources.drawPipeline||resources.beforeDraw)throw std::runtime_error("HDR2 draw hook inputs/conflict");auto self=shared_from_this();const auto layout=resources.layout;
        VulkanScenePipelineSelectorCandidate select=[self,hdrPipeline](const auto&d,VkPipeline fallback){return self->resource(d)==SIZE_MAX?fallback:hdrPipeline;};
        std::function<void(const SceneGpuDrawCandidate&)>before=[self,layout,command](const auto&d){const auto n=self->resource(d);if(n==SIZE_MAX)return;const VkDeviceSize zero=0;load<PFN_vkCmdBindVertexBuffers>(self->library_,"vkCmdBindVertexBuffers")(command,1,1,&self->uvs_,&zero);const auto set=self->sets_.at(n);load<PFN_vkCmdBindDescriptorSets>(self->library_,"vkCmdBindDescriptorSets")(command,VK_PIPELINE_BIND_POINT_GRAPHICS,layout,1,1,&set,0,nullptr);load<PFN_vkCmdPushConstants>(self->library_,"vkCmdPushConstants")(command,layout,VK_SHADER_STAGE_FRAGMENT_BIT,144,16,self->push_.data());};
        resources.drawPipeline.swap(select);resources.beforeDraw.swap(before);
    }
};
