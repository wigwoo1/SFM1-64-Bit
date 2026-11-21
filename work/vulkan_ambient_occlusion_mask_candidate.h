#pragma once
#include "ambient_occlusion_view_candidate.h"
#include "editor_shader_resources_candidate.h"
#include <fstream>

inline bool vulkanAmbientOcclusionShadingRequestedCandidate() {
    char value[8]{};
    return GetEnvironmentVariableA("SFM64_DIAGNOSTIC_AO_MASK_READBACK",value,sizeof(value))==5 &&
           std::strcmp(value,"shade")==0;
}

// Caller waits for its frame fence before resizing, updating descriptors or
// destroying the owner. Both rendering and compute use the same command buffer.
class VulkanAmbientOcclusionMaskCandidate {
    HMODULE library_;
    VkDevice device_;
    VkPhysicalDevice physical_;
    struct Texture {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
    };
    Texture noise_, mask_;
    uint32_t width_ = 0, height_ = 0;
    uint32_t noiseExtent_;
    VkSampler clamp_ = VK_NULL_HANDLE, tile_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptors_ = VK_NULL_HANDLE;
    VkDescriptorPool pool_ = VK_NULL_HANDLE;
    VkDescriptorSet set_ = VK_NULL_HANDLE, shadingSet_ = VK_NULL_HANDLE;
    VkPipelineLayout layout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    bool noiseRecorded_ = false;

    template<class F> F proc(const char* name) const { return load<F>(library_, name); }
    static void check(VkResult result) {
        if (result != VK_SUCCESS)
            throw std::runtime_error("AO mask Vulkan result " + std::to_string(result));
    }
    void destroy(Texture& texture) const noexcept {
        if (texture.view) proc<PFN_vkDestroyImageView>("vkDestroyImageView")(device_, texture.view, nullptr);
        if (texture.image) proc<PFN_vkDestroyImage>("vkDestroyImage")(device_, texture.image, nullptr);
        if (texture.memory) proc<PFN_vkFreeMemory>("vkFreeMemory")(device_, texture.memory, nullptr);
        texture = {};
    }
    void reset() noexcept {
        if (pipeline_) proc<PFN_vkDestroyPipeline>("vkDestroyPipeline")(device_, pipeline_, nullptr);
        if (layout_) proc<PFN_vkDestroyPipelineLayout>("vkDestroyPipelineLayout")(device_, layout_, nullptr);
        if (pool_) proc<PFN_vkDestroyDescriptorPool>("vkDestroyDescriptorPool")(device_, pool_, nullptr);
        if (descriptors_) proc<PFN_vkDestroyDescriptorSetLayout>("vkDestroyDescriptorSetLayout")(device_, descriptors_, nullptr);
        if (tile_) proc<PFN_vkDestroySampler>("vkDestroySampler")(device_, tile_, nullptr);
        if (clamp_) proc<PFN_vkDestroySampler>("vkDestroySampler")(device_, clamp_, nullptr);
        destroy(mask_);
        destroy(noise_);
    }
    Texture texture(VkFormat format, VkImageUsageFlags usage, uint32_t w, uint32_t h) const {
        VkImageFormatProperties formatLimits{};
        check(proc<PFN_vkGetPhysicalDeviceImageFormatProperties>("vkGetPhysicalDeviceImageFormatProperties")(
            physical_, format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, 0, &formatLimits));
        if (!w || !h || w > formatLimits.maxExtent.width || h > formatLimits.maxExtent.height)
            throw std::runtime_error("AO image extent unsupported");
        Texture result;
        try {
            VkImageCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            info.imageType = VK_IMAGE_TYPE_2D;
            info.format = format;
            info.extent = {w,h,1};
            info.mipLevels = info.arrayLayers = 1;
            info.samples = VK_SAMPLE_COUNT_1_BIT;
            info.tiling = VK_IMAGE_TILING_OPTIMAL;
            info.usage = usage;
            check(proc<PFN_vkCreateImage>("vkCreateImage")(device_, &info, nullptr, &result.image));
            VkMemoryRequirements requirements{};
            proc<PFN_vkGetImageMemoryRequirements>("vkGetImageMemoryRequirements")(device_, result.image, &requirements);
            VkPhysicalDeviceMemoryProperties properties{};
            proc<PFN_vkGetPhysicalDeviceMemoryProperties>("vkGetPhysicalDeviceMemoryProperties")(physical_, &properties);
            uint32_t type = UINT32_MAX;
            for (uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
                if ((requirements.memoryTypeBits & (1u << i)) &&
                    (properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
                    type = i;
                    break;
                }
            }
            if (type == UINT32_MAX) throw std::runtime_error("AO image device memory not available");
            VkMemoryAllocateInfo allocation{};
            allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocation.allocationSize = requirements.size;
            allocation.memoryTypeIndex = type;
            check(proc<PFN_vkAllocateMemory>("vkAllocateMemory")(device_, &allocation, nullptr, &result.memory));
            check(proc<PFN_vkBindImageMemory>("vkBindImageMemory")(device_, result.image, result.memory, 0));
            VkImageViewCreateInfo view{};
            view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view.image = result.image;
            view.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view.format = format;
            view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
            check(proc<PFN_vkCreateImageView>("vkCreateImageView")(device_, &view, nullptr, &result.view));
            return result;
        } catch (...) {
            destroy(result);
            throw;
        }
    }
    VkSampler sampler(VkSamplerAddressMode address) const {
        VkSamplerCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.magFilter = info.minFilter = VK_FILTER_NEAREST;
        info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        info.addressModeU = info.addressModeV = info.addressModeW = address;
        VkSampler result{};
        check(proc<PFN_vkCreateSampler>("vkCreateSampler")(device_, &info, nullptr, &result));
        return result;
    }
    static constexpr VkDescriptorType types_[6]{
        VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        VK_DESCRIPTOR_TYPE_SAMPLER, VK_DESCRIPTOR_TYPE_SAMPLER,
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
    };
    void createLayout(VkDescriptorSetLayout shadingLayout) {
        VkDescriptorSetLayoutBinding bindings[6]{};
        for (uint32_t i = 0; i < 6; ++i)
            bindings[i] = {i,types_[i],1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr};
        VkDescriptorSetLayoutCreateInfo descriptorLayout{};
        descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorLayout.bindingCount = 6;
        descriptorLayout.pBindings = bindings;
        check(proc<PFN_vkCreateDescriptorSetLayout>("vkCreateDescriptorSetLayout")(
            device_, &descriptorLayout, nullptr, &descriptors_));
        const VkDescriptorPoolSize sizes[]{
            {types_[0],shadingLayout?3u:2u}, {types_[2],shadingLayout?3u:2u}, {types_[4],1}, {types_[5],1}
        };
        VkDescriptorPoolCreateInfo pool{};
        pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool.maxSets = shadingLayout?2:1;
        pool.poolSizeCount = 4;
        pool.pPoolSizes = sizes;
        check(proc<PFN_vkCreateDescriptorPool>("vkCreateDescriptorPool")(device_, &pool, nullptr, &pool_));
        VkDescriptorSetAllocateInfo allocate{};
        allocate.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocate.descriptorPool = pool_;
        allocate.descriptorSetCount = 1;
        allocate.pSetLayouts = &descriptors_;
        check(proc<PFN_vkAllocateDescriptorSets>("vkAllocateDescriptorSets")(device_, &allocate, &set_));
        if (shadingLayout) {
            allocate.pSetLayouts = &shadingLayout;
            check(proc<PFN_vkAllocateDescriptorSets>("vkAllocateDescriptorSets")(device_, &allocate, &shadingSet_));
        }
        VkPipelineLayoutCreateInfo layout{};
        layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout.setLayoutCount = 1;
        layout.pSetLayouts = &descriptors_;
        check(proc<PFN_vkCreatePipelineLayout>("vkCreatePipelineLayout")(device_, &layout, nullptr, &layout_));
    }
    void createPipeline() {
        std::ifstream file(editorShaderResourcePathCandidate("build/session-core/sfm64-ambient-occlusion-mask-candidate.spv"),
                           std::ios::binary | std::ios::ate);
        const auto bytes = static_cast<std::streamoff>(file.tellg());
        if (bytes < 20 || bytes > 1048576 || bytes % 4) throw std::runtime_error("AO mask shader size");
        std::vector<uint32_t> words(static_cast<size_t>(bytes) / 4);
        file.seekg(0);
        if (!file.read(reinterpret_cast<char*>(words.data()), bytes) || words[0] != 0x07230203)
            throw std::runtime_error("AO mask shader framing");
        const auto create = proc<PFN_vkCreateComputePipelines>("vkCreateComputePipelines");
        const auto destroyShader = proc<PFN_vkDestroyShaderModule>("vkDestroyShaderModule");
        VkShaderModuleCreateInfo moduleInfo{};
        moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleInfo.codeSize = static_cast<size_t>(bytes);
        moduleInfo.pCode = words.data();
        VkShaderModule shader{};
        check(proc<PFN_vkCreateShaderModule>("vkCreateShaderModule")(device_, &moduleInfo, nullptr, &shader));
        VkComputePipelineCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        info.stage.module = shader;
        info.stage.pName = "main";
        info.layout = layout_;
        const VkResult result = create(device_, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline_);
        destroyShader(device_, shader, nullptr);
        check(result);
    }
    void transition(VkCommandBuffer command, const Texture& texture, VkImageLayout oldLayout, VkImageLayout newLayout,
                    VkPipelineStageFlags sourceStage, VkPipelineStageFlags targetStage,
                    VkAccessFlags sourceAccess, VkAccessFlags targetAccess) const {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.srcAccessMask = sourceAccess;
        barrier.dstAccessMask = targetAccess;
        barrier.image = texture.image;
        barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        proc<PFN_vkCmdPipelineBarrier>("vkCmdPipelineBarrier")(command, sourceStage, targetStage,
                                                             0, 0, nullptr, 0, nullptr, 1, &barrier);
    }
    void recordNoise(VkCommandBuffer command, VkBuffer staging) {
        if (noiseRecorded_) return;
        if (!staging) throw std::runtime_error("Missing AO noise staging buffer");
        transition(command,noise_,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,VK_ACCESS_TRANSFER_WRITE_BIT);
        VkBufferImageCopy copy{};
        copy.imageExtent = {noiseExtent_,noiseExtent_,1};
        copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};
        proc<PFN_vkCmdCopyBufferToImage>("vkCmdCopyBufferToImage")(
            command,staging,noise_.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
        transition(command,noise_,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                   VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                   VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT);
        noiseRecorded_ = true;
    }
public:
    VulkanAmbientOcclusionMaskCandidate(HMODULE library,VkDevice device,VkPhysicalDevice physical,uint32_t family,
                                       VkDescriptorSetLayout shadingLayout = VK_NULL_HANDLE,
                                       uint32_t noiseExtent = ambientOcclusionNoiseExtentCandidate)
        : library_(library),device_(device),physical_(physical),noiseExtent_(noiseExtent) {
        try {
            const auto queueProperties = proc<PFN_vkGetPhysicalDeviceQueueFamilyProperties>("vkGetPhysicalDeviceQueueFamilyProperties");
            uint32_t count=0;
            queueProperties(physical_,&count,nullptr);
            std::vector<VkQueueFamilyProperties> families(count);
            queueProperties(physical_,&count,families.data());
            if (family>=count || !(families[family].queueFlags&VK_QUEUE_COMPUTE_BIT))
                throw std::runtime_error("AO mask requires compute support on the viewport queue");
            noise_=texture(VK_FORMAT_R32G32B32A32_SFLOAT,VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                           noiseExtent_,noiseExtent_);
            clamp_=sampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
            tile_=sampler(VK_SAMPLER_ADDRESS_MODE_REPEAT);
            createLayout(shadingLayout);
            createPipeline();
        } catch (...) {
            reset();
            throw;
        }
    }
    VulkanAmbientOcclusionMaskCandidate(const VulkanAmbientOcclusionMaskCandidate&) = delete;
    ~VulkanAmbientOcclusionMaskCandidate() { reset(); }
    VkDescriptorSet shadingSet() const { return shadingSet_; }

    void bind(VkImageView front,VkBuffer uniform,uint32_t width,uint32_t height) {
        if (!front || !uniform) throw std::runtime_error("AO mask input bindings missing");
        if (width!=width_ || height!=height_) {
            auto next=texture(VK_FORMAT_R32_SFLOAT,
                VK_IMAGE_USAGE_STORAGE_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT,width,height);
            destroy(mask_);
            mask_=next;
            width_=width;
            height_=height;
        }
        const VkDescriptorImageInfo images[]{
            {VK_NULL_HANDLE,front,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
            {VK_NULL_HANDLE,noise_.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
            {clamp_,VK_NULL_HANDLE,VK_IMAGE_LAYOUT_UNDEFINED},
            {tile_,VK_NULL_HANDLE,VK_IMAGE_LAYOUT_UNDEFINED},
            {VK_NULL_HANDLE,mask_.view,VK_IMAGE_LAYOUT_GENERAL},
        };
        const VkDescriptorBufferInfo buffer{uniform,0,sizeof(AmbientOcclusionViewCandidate)};
        VkWriteDescriptorSet writes[6]{};
        for (uint32_t i=0;i<6;++i) {
            writes[i].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[i].dstSet=set_;
            writes[i].dstBinding=i;
            writes[i].descriptorCount=1;
            writes[i].descriptorType=types_[i];
            if (i<5) writes[i].pImageInfo=&images[i];
            else writes[i].pBufferInfo=&buffer;
        }
        proc<PFN_vkUpdateDescriptorSets>("vkUpdateDescriptorSets")(device_,6,writes,0,nullptr);
        if (shadingSet_) {
            const VkDescriptorImageInfo shadeImages[]{
                {VK_NULL_HANDLE,mask_.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
                {clamp_,VK_NULL_HANDLE,VK_IMAGE_LAYOUT_UNDEFINED},
            };
            VkWriteDescriptorSet shadeWrites[2]{};
            for (uint32_t i=0;i<2;++i) {
                shadeWrites[i].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                shadeWrites[i].dstSet=shadingSet_;
                shadeWrites[i].dstBinding=i;
                shadeWrites[i].descriptorCount=1;
                shadeWrites[i].descriptorType=i==0?VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:VK_DESCRIPTOR_TYPE_SAMPLER;
                shadeWrites[i].pImageInfo=&shadeImages[i];
            }
            proc<PFN_vkUpdateDescriptorSets>("vkUpdateDescriptorSets")(device_,2,shadeWrites,0,nullptr);
        }
    }

    void record(VkCommandBuffer command,VkBuffer noiseStaging,VkBuffer readback) {
        if (!mask_.image || !readback) throw std::runtime_error("AO mask readback resources missing");
        recordNoise(command,noiseStaging);
        transition(command,mask_,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_GENERAL,
                   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,VK_ACCESS_SHADER_WRITE_BIT);
        proc<PFN_vkCmdBindPipeline>("vkCmdBindPipeline")(command,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline_);
        proc<PFN_vkCmdBindDescriptorSets>("vkCmdBindDescriptorSets")(
            command,VK_PIPELINE_BIND_POINT_COMPUTE,layout_,0,1,&set_,0,nullptr);
        proc<PFN_vkCmdDispatch>("vkCmdDispatch")(command,(width_+7)/8,(height_+7)/8,1);
        transition(command,mask_,VK_IMAGE_LAYOUT_GENERAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,
                   VK_ACCESS_SHADER_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT);
        VkBufferImageCopy copy{};
        copy.imageExtent={width_,height_,1};
        copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};
        proc<PFN_vkCmdCopyImageToBuffer>("vkCmdCopyImageToBuffer")(
            command,mask_.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,readback,1,&copy);
        VkMemoryBarrier host{};
        host.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
        host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
        proc<PFN_vkCmdPipelineBarrier>("vkCmdPipelineBarrier")(command,VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_HOST_BIT,0,1,&host,0,nullptr,0,nullptr);
        if (shadingSet_)
            transition(command,mask_,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                       VK_PIPELINE_STAGE_TRANSFER_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_ACCESS_TRANSFER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT,
                       VK_ACCESS_SHADER_READ_BIT);
    }
};
