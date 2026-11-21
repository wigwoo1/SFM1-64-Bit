#pragma once
#include "editor_shader_resources_candidate.h"
#include "source_render_sample_accumulation_candidate.h"
#include <charconv>
#include <fstream>

inline uint32_t vulkanRenderSampleDiagnosticCountCandidate() {
    char text[12]{};
    const auto length = GetEnvironmentVariableA("SFM64_DIAGNOSTIC_RENDER_SAMPLE_COUNT",text,sizeof(text));
    if (!length) return 0;
    uint32_t count = 0;
    if (length >= sizeof(text)) throw std::runtime_error("Render sample count must be 1 through 256");
    const auto parsed = std::from_chars(text,text+length,count);
    if (parsed.ec != std::errc{} || parsed.ptr != text+length || !count || count>256)
        throw std::runtime_error("Render sample count must be 1 through 256");
    return count;
}

struct VulkanRenderSamplingOptionsCandidate {
    uint32_t explicitCount;
    bool sessionRefinement;
    bool enabled() const { return sessionRefinement || explicitCount != 0; }
};

inline VulkanRenderSamplingOptionsCandidate vulkanRenderSamplingOptionsCandidate() {
    char text[2]{};
    const auto length=GetEnvironmentVariableA("SFM64_DIAGNOSTIC_SESSION_REFINEMENT",text,sizeof(text));
    if(length&&(length!=1||text[0]!='1'))
        throw std::runtime_error("Expected 1 for SFM64_DIAGNOSTIC_SESSION_REFINEMENT");
    const auto explicitCount=vulkanRenderSampleDiagnosticCountCandidate();
    if(length&&explicitCount)
        throw std::runtime_error("Session refinement and an explicit sample count cannot be combined");
    return {explicitCount,length!=0};
}

// External scratch buffers are device-owned by the draw loop. All records
// and the final transfer share its command buffer and fence.
class VulkanRenderSampleAccumulatorCandidate {
    HMODULE library_;
    VkDevice device_;
    VkImage source_;
    VkBuffer sample_;
    std::array<VkBuffer,2> averages_;
    VkExtent3D extent_;
    VkDeviceSize bytes_;
    uint32_t pixels_;
    VkDescriptorSetLayout descriptors_ = VK_NULL_HANDLE;
    VkDescriptorPool pool_ = VK_NULL_HANDLE;
    std::array<VkDescriptorSet,2> sets_{};
    VkPipelineLayout layout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkShaderModule module_ = VK_NULL_HANDLE;

    struct Push { uint32_t inputOffset, pixels; float currentWeight, previousWeight; };
    static_assert(sizeof(Push)==16);

    template<class F> F fn(const char* name) const { return load<F>(library_,name); }
    static void check(VkResult status) {
        if (status != VK_SUCCESS)
            throw std::runtime_error("Vulkan sample accumulation result "+std::to_string(status));
    }
    void reset() noexcept {
        if (pipeline_) fn<PFN_vkDestroyPipeline>("vkDestroyPipeline")(device_,pipeline_,nullptr);
        if (module_) fn<PFN_vkDestroyShaderModule>("vkDestroyShaderModule")(device_,module_,nullptr);
        if (layout_) fn<PFN_vkDestroyPipelineLayout>("vkDestroyPipelineLayout")(device_,layout_,nullptr);
        if (pool_) fn<PFN_vkDestroyDescriptorPool>("vkDestroyDescriptorPool")(device_,pool_,nullptr);
        if (descriptors_) fn<PFN_vkDestroyDescriptorSetLayout>("vkDestroyDescriptorSetLayout")(device_,descriptors_,nullptr);
    }
    void memoryBarrier(VkCommandBuffer command,VkPipelineStageFlags from,VkPipelineStageFlags to,
                       VkAccessFlags readsOrWrites,VkAccessFlags nextAccess) const {
        VkMemoryBarrier info{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        info.srcAccessMask=readsOrWrites;
        info.dstAccessMask=nextAccess;
        fn<PFN_vkCmdPipelineBarrier>("vkCmdPipelineBarrier")(command,from,to,0,1,&info,0,nullptr,0,nullptr);
    }
    void transition(VkCommandBuffer command,VkImageLayout from,VkImageLayout to,
                    VkPipelineStageFlags fromStage,VkPipelineStageFlags toStage,
                    VkAccessFlags fromAccess,VkAccessFlags toAccess) const {
        VkImageMemoryBarrier info{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        info.srcAccessMask=fromAccess;
        info.dstAccessMask=toAccess;
        info.oldLayout=from;
        info.newLayout=to;
        info.srcQueueFamilyIndex=info.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        info.image=source_;
        info.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        fn<PFN_vkCmdPipelineBarrier>("vkCmdPipelineBarrier")(command,fromStage,toStage,0,0,nullptr,0,nullptr,1,&info);
    }
public:
    VulkanRenderSampleAccumulatorCandidate(HMODULE library,VkDevice device,VkPhysicalDevice physical,
            VkImage source,VkBuffer sample,std::array<VkBuffer,2> averages,uint32_t width,uint32_t height)
        : library_(library),device_(device),source_(source),sample_(sample),averages_(averages),
          extent_{width,height,1},bytes_(VkDeviceSize(width)*height*16),pixels_(width*height) {
        VkPhysicalDeviceProperties properties{};
        fn<PFN_vkGetPhysicalDeviceProperties>("vkGetPhysicalDeviceProperties")(physical,&properties);
        if (!source || !sample || !averages[0] || !averages[1] || !pixels_ ||
            bytes_>properties.limits.maxStorageBufferRange ||
            (pixels_+63)/64>properties.limits.maxComputeWorkGroupCount[0])
            throw std::runtime_error("Render sample accumulation extent is unsupported");
        try {
            VkDescriptorSetLayoutBinding bindings[3]{};
            for (uint32_t i=0;i<3;++i) bindings[i]={i,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr};
            VkDescriptorSetLayoutCreateInfo descriptors{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
            descriptors.bindingCount=3;
            descriptors.pBindings=bindings;
            check(fn<PFN_vkCreateDescriptorSetLayout>("vkCreateDescriptorSetLayout")(device_,&descriptors,nullptr,&descriptors_));
            const VkDescriptorPoolSize capacity{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,6};
            VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
            pool.maxSets=2;
            pool.poolSizeCount=1;
            pool.pPoolSizes=&capacity;
            check(fn<PFN_vkCreateDescriptorPool>("vkCreateDescriptorPool")(device_,&pool,nullptr,&pool_));
            const VkDescriptorSetLayout layouts[]{descriptors_,descriptors_};
            VkDescriptorSetAllocateInfo sets{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            sets.descriptorPool=pool_;
            sets.descriptorSetCount=2;
            sets.pSetLayouts=layouts;
            check(fn<PFN_vkAllocateDescriptorSets>("vkAllocateDescriptorSets")(device_,&sets,sets_.data()));
            for (size_t ping=0;ping<2;++ping) {
                const VkDescriptorBufferInfo buffers[]{{sample_,0,bytes_},{averages_[ping],0,bytes_},{averages_[1-ping],0,bytes_}};
                VkWriteDescriptorSet writes[3]{};
                for (uint32_t i=0;i<3;++i) {
                    writes[i].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    writes[i].dstSet=sets_[ping];
                    writes[i].dstBinding=i;
                    writes[i].descriptorCount=1;
                    writes[i].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    writes[i].pBufferInfo=&buffers[i];
                }
                fn<PFN_vkUpdateDescriptorSets>("vkUpdateDescriptorSets")(device_,3,writes,0,nullptr);
            }
            const VkPushConstantRange push{VK_SHADER_STAGE_COMPUTE_BIT,0,sizeof(Push)};
            VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
            layout.setLayoutCount=1;
            layout.pSetLayouts=&descriptors_;
            layout.pushConstantRangeCount=1;
            layout.pPushConstantRanges=&push;
            check(fn<PFN_vkCreatePipelineLayout>("vkCreatePipelineLayout")(device_,&layout,nullptr,&layout_));
            std::ifstream file(editorShaderResourcePathCandidate("sfm64-render-sample-accumulation-candidate.spv"),
                               std::ios::binary|std::ios::ate);
            const auto size=file.tellg();
            if (size<20 || size>1048576 || size%4) throw std::runtime_error("Missing or invalid render-sample shader");
            std::vector<uint32_t> words(size_t(size)/4);
            file.seekg(0);
            if (!file.read(reinterpret_cast<char*>(words.data()),size) || words.front()!=0x07230203)
                throw std::runtime_error("Render-sample shader load failed");
            VkShaderModuleCreateInfo module{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
            module.codeSize=size_t(size);
            module.pCode=words.data();
            check(fn<PFN_vkCreateShaderModule>("vkCreateShaderModule")(device_,&module,nullptr,&module_));
            VkComputePipelineCreateInfo pipeline{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
            pipeline.stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            pipeline.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;
            pipeline.stage.module=module_;
            pipeline.stage.pName="main";
            pipeline.layout=layout_;
            check(fn<PFN_vkCreateComputePipelines>("vkCreateComputePipelines")(device_,VK_NULL_HANDLE,1,&pipeline,nullptr,&pipeline_));
        } catch (...) { reset(); throw; }
    }
    VulkanRenderSampleAccumulatorCandidate(const VulkanRenderSampleAccumulatorCandidate&)=delete;
    ~VulkanRenderSampleAccumulatorCandidate() { reset(); }

    void record(VkCommandBuffer command,uint32_t ordinal,VkBuffer finalReadback) const {
        constexpr auto transfer=VK_PIPELINE_STAGE_TRANSFER_BIT;
        constexpr auto compute=VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        if (!ordinal) fn<PFN_vkCmdFillBuffer>("vkCmdFillBuffer")(command,averages_[0],0,bytes_,0);
        transition(command,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,transfer,
                   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT);
        VkBufferImageCopy region{};
        region.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};
        region.imageExtent=extent_;
        fn<PFN_vkCmdCopyImageToBuffer>("vkCmdCopyImageToBuffer")(
            command,source_,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,sample_,1,&region);
        memoryBarrier(command,transfer,compute,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT);
        const auto weights=sourceRenderSampleWeightsCandidate(ordinal);
        const Push push{0,pixels_,weights[0],weights[1]};
        const auto ping=ordinal%2;
        fn<PFN_vkCmdBindPipeline>("vkCmdBindPipeline")(command,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline_);
        fn<PFN_vkCmdBindDescriptorSets>("vkCmdBindDescriptorSets")(
            command,VK_PIPELINE_BIND_POINT_COMPUTE,layout_,0,1,&sets_[ping],0,nullptr);
        fn<PFN_vkCmdPushConstants>("vkCmdPushConstants")(command,layout_,VK_SHADER_STAGE_COMPUTE_BIT,0,sizeof(push),&push);
        fn<PFN_vkCmdDispatch>("vkCmdDispatch")(command,(pixels_+63)/64,1,1);
        memoryBarrier(command,compute,compute|transfer,VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT,
                      VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_TRANSFER_READ_BIT|VK_ACCESS_TRANSFER_WRITE_BIT);
        if (finalReadback) {
            transition(command,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       transfer,transfer,VK_ACCESS_TRANSFER_READ_BIT,VK_ACCESS_TRANSFER_WRITE_BIT);
            fn<PFN_vkCmdCopyBufferToImage>("vkCmdCopyBufferToImage")(
                command,averages_[1-ping],source_,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&region);
            const VkBufferCopy copy{0,0,bytes_};
            fn<PFN_vkCmdCopyBuffer>("vkCmdCopyBuffer")(command,averages_[1-ping],finalReadback,1,&copy);
        }
    }
};
