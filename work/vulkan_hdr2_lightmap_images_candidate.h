#pragma once
#include "scene_hdr2_lightmap_candidate.h"
#include "vulkan_material_images_candidate.h"

// Device/queue confined owner. Call submitted only after successful submission,
// completed only after its fence. A recorded-but-discarded command must be reset
// before discardRecording or destruction; never submit stale recorded handles.
class VulkanHdr2LightmapImagesCandidate {
public:
    enum class State {Fresh,Recorded,InFlight,Ready};
private:
    HMODULE library_;VkDevice device_;
    std::shared_ptr<const SceneHdr2PlanCandidate> plan_;
    std::unique_ptr<VulkanMaterialImagesCandidate> images_;
    VkFormat format_=VK_FORMAT_UNDEFINED;
    State state_=State::Fresh;VkCommandBuffer command_=VK_NULL_HANDLE;
    VulkanHdr2LightmapImagesCandidate(HMODULE library,VkDevice device,VkPhysicalDevice physical,
        std::shared_ptr<const SceneHdr2PlanCandidate> plan):library_(library),device_(device),plan_(std::move(plan)),images_(std::make_unique<VulkanMaterialImagesCandidate>(library,device,physical)){}
public:
    VulkanHdr2LightmapImagesCandidate(const VulkanHdr2LightmapImagesCandidate&)=delete;
    ~VulkanHdr2LightmapImagesCandidate(){
        // Exceptional retirement safety. Normal users retain through the fence
        // and call completed; destruction then requires no device-wide wait.
        if(state_==State::InFlight)load<PFN_vkDeviceWaitIdle>(library_,"vkDeviceWaitIdle")(device_);
    }
    State state()const noexcept{return state_;}
    VkFormat format()const noexcept{return format_;}
    const auto& plan()const noexcept{return plan_;}
    const auto& entries()const noexcept{return images_->entries;}
    static std::shared_ptr<VulkanHdr2LightmapImagesCandidate> prepare(HMODULE library,VkDevice device,VkPhysicalDevice physical,
        std::shared_ptr<const SceneHdr2PlanCandidate> plan,const std::function<void(const char*)>&hook={}){
        if(!library||!device||!physical||!plan||plan->policy.selectedHdrType!=2||
           (plan->policy.encoding!=113&&plan->policy.encoding!=36))throw std::runtime_error("HDR2 Vulkan explicit plan/device required");
        auto stage=[&](const char*s){if(hook)hook(s);};
        const VkFormat format=plan->policy.encoding==113?VK_FORMAT_R16G16B16A16_SFLOAT:VK_FORMAT_R16G16B16A16_UNORM;
        const VkImageUsageFlags usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
        VkFormatProperties features{};load<PFN_vkGetPhysicalDeviceFormatProperties>(library,"vkGetPhysicalDeviceFormatProperties")(physical,format,&features);
        const VkFormatFeatureFlags required=VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT|VK_FORMAT_FEATURE_TRANSFER_SRC_BIT|VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
        if((features.optimalTilingFeatures&required)!=required)throw std::runtime_error("HDR2 Vulkan format features unavailable");
        VkImageFormatProperties limits{};VulkanMaterialImagesCandidate::check(load<PFN_vkGetPhysicalDeviceImageFormatProperties>(library,"vkGetPhysicalDeviceImageFormatProperties")(physical,format,VK_IMAGE_TYPE_2D,VK_IMAGE_TILING_OPTIMAL,usage,0,&limits));
        for(const auto&f:plan->faces){const auto&p=f.page;if((!f.accumulated&&!(f.directionalStep>0))||!p.width||!p.height||p.width>limits.maxExtent.width||p.height>limits.maxExtent.height||!(limits.sampleCounts&VK_SAMPLE_COUNT_1_BIT)||!limits.maxMipLevels||uint64_t(p.width)*p.height>SIZE_MAX/8||f.bytes.size()!=uint64_t(p.width)*p.height*8)throw std::runtime_error("HDR2 Vulkan page extent/bytes unsupported");}
        auto next=std::shared_ptr<VulkanHdr2LightmapImagesCandidate>(new VulkanHdr2LightmapImagesCandidate(library,device,physical,std::move(plan)));next->format_=format;
        VkPhysicalDeviceMemoryProperties properties{};load<PFN_vkGetPhysicalDeviceMemoryProperties>(library,"vkGetPhysicalDeviceMemoryProperties")(physical,&properties);
        auto allocate=[&](VkMemoryRequirements req,VkMemoryPropertyFlags flags,VkDeviceMemory&memory){uint32_t type=UINT32_MAX;for(uint32_t n=0;n<properties.memoryTypeCount;++n)if((req.memoryTypeBits&(1u<<n))&&(properties.memoryTypes[n].propertyFlags&flags)==flags){type=n;break;}if(type==UINT32_MAX)throw std::runtime_error("HDR2 Vulkan memory type");VkMemoryAllocateInfo i{};i.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;i.allocationSize=req.size;i.memoryTypeIndex=type;VulkanMaterialImagesCandidate::check(load<PFN_vkAllocateMemory>(library,"vkAllocateMemory")(device,&i,nullptr,&memory));};
        next->images_->entries.resize(next->plan_->faces.size());
        for(size_t n=0;n<next->plan_->faces.size();++n){const auto&f=next->plan_->faces[n];auto&e=next->images_->entries[n];e.width=f.page.width;e.height=f.page.height;e.format=format;
            VkBufferCreateInfo bi{};bi.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;bi.size=f.bytes.size();bi.usage=VK_BUFFER_USAGE_TRANSFER_SRC_BIT;VulkanMaterialImagesCandidate::check(load<PFN_vkCreateBuffer>(library,"vkCreateBuffer")(device,&bi,nullptr,&e.staging));stage("staging-created");
            VkMemoryRequirements req{};load<PFN_vkGetBufferMemoryRequirements>(library,"vkGetBufferMemoryRequirements")(device,e.staging,&req);allocate(req,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,e.stagingMemory);stage("staging-allocated");VulkanMaterialImagesCandidate::check(load<PFN_vkBindBufferMemory>(library,"vkBindBufferMemory")(device,e.staging,e.stagingMemory,0));
            void*mapped=nullptr;VulkanMaterialImagesCandidate::check(load<PFN_vkMapMemory>(library,"vkMapMemory")(device,e.stagingMemory,0,f.bytes.size(),0,&mapped));std::memcpy(mapped,f.bytes.data(),f.bytes.size());load<PFN_vkUnmapMemory>(library,"vkUnmapMemory")(device,e.stagingMemory);stage("staging-filled");
            VkImageCreateInfo ii{};ii.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;ii.imageType=VK_IMAGE_TYPE_2D;ii.format=format;ii.extent={e.width,e.height,1};ii.mipLevels=ii.arrayLayers=1;ii.samples=VK_SAMPLE_COUNT_1_BIT;ii.tiling=VK_IMAGE_TILING_OPTIMAL;ii.usage=usage;VulkanMaterialImagesCandidate::check(load<PFN_vkCreateImage>(library,"vkCreateImage")(device,&ii,nullptr,&e.image));stage("image-created");
            load<PFN_vkGetImageMemoryRequirements>(library,"vkGetImageMemoryRequirements")(device,e.image,&req);allocate(req,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,e.imageMemory);stage("image-allocated");VulkanMaterialImagesCandidate::check(load<PFN_vkBindImageMemory>(library,"vkBindImageMemory")(device,e.image,e.imageMemory,0));
            VkImageViewCreateInfo vi{};vi.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;vi.image=e.image;vi.viewType=VK_IMAGE_VIEW_TYPE_2D;vi.format=format;vi.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};VulkanMaterialImagesCandidate::check(load<PFN_vkCreateImageView>(library,"vkCreateImageView")(device,&vi,nullptr,&e.view));stage("view-created");
        }stage("images-ready");return next;
    }
    void recordUpload(VkCommandBuffer command){
        if(!command)throw std::runtime_error("HDR2 upload command missing");
        if(state_==State::Ready)return;
        if(state_!=State::Fresh)throw std::runtime_error("HDR2 upload already recorded/in flight");
        images_->upload(command);command_=command;state_=State::Recorded;
    }
    void discardRecording(VkCommandBuffer command){if(state_!=State::Recorded||command!=command_)throw std::runtime_error("HDR2 discard identity/state");command_=VK_NULL_HANDLE;state_=State::Fresh;}
    void submitted(VkCommandBuffer command){if(state_!=State::Recorded||command!=command_)throw std::runtime_error("HDR2 submit identity/state");state_=State::InFlight;}
    void completed(){if(state_!=State::InFlight)throw std::runtime_error("HDR2 completion without submission");state_=State::Ready;command_=VK_NULL_HANDLE;}
};
