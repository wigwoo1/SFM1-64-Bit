#pragma once
#include "editor_shader_resources_candidate.h"
#include <array>
#include <fstream>

// Keep linear lighting and blending in float storage. Encode once into an
// eight-bit result, then use that image's UNORM format for capture/presentation.
inline VkFormat vulkanViewportSceneFormatCandidate(bool hdr2, bool diagnosticSrgb) {
    return hdr2 ? VK_FORMAT_R16G16B16A16_SFLOAT :
        diagnosticSrgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
}

// The owner must complete all submissions (including presentation reads) before
// destruction. record() shares the editor's existing command buffer and fence.
class VulkanViewportOutputCopyCandidate {
    HMODULE library_;
    VkDevice device_;
    VkImage source_;
    uint32_t width_, height_;
    VkImage image_ = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;
    VkImageView view_ = VK_NULL_HANDLE;
    VkRenderPass pass_ = VK_NULL_HANDLE;
    VkFramebuffer framebuffer_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorLayout_ = VK_NULL_HANDLE;
    VkDescriptorPool pool_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptor_ = VK_NULL_HANDLE;
    VkPipelineLayout layout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    std::array<VkShaderModule, 2> shaders_{};
    bool recorded_ = false;

    template<class Function> Function function(const char* name) const {
        return load<Function>(library_, name);
    }
    static void check(VkResult result) {
        if (result != VK_SUCCESS)
            throw std::runtime_error("viewport output copy result " + std::to_string(result));
    }
    void reset() noexcept {
        if (pipeline_) function<PFN_vkDestroyPipeline>("vkDestroyPipeline")(device_, pipeline_, nullptr);
        if (layout_) function<PFN_vkDestroyPipelineLayout>("vkDestroyPipelineLayout")(device_, layout_, nullptr);
        if (pool_) function<PFN_vkDestroyDescriptorPool>("vkDestroyDescriptorPool")(device_, pool_, nullptr);
        if (descriptorLayout_) function<PFN_vkDestroyDescriptorSetLayout>("vkDestroyDescriptorSetLayout")(device_, descriptorLayout_, nullptr);
        if (framebuffer_) function<PFN_vkDestroyFramebuffer>("vkDestroyFramebuffer")(device_, framebuffer_, nullptr);
        if (pass_) function<PFN_vkDestroyRenderPass>("vkDestroyRenderPass")(device_, pass_, nullptr);
        for (auto shader : shaders_) if (shader) function<PFN_vkDestroyShaderModule>("vkDestroyShaderModule")(device_, shader, nullptr);
        if (view_) function<PFN_vkDestroyImageView>("vkDestroyImageView")(device_, view_, nullptr);
        if (image_) function<PFN_vkDestroyImage>("vkDestroyImage")(device_, image_, nullptr);
        if (memory_) function<PFN_vkFreeMemory>("vkFreeMemory")(device_, memory_, nullptr);
    }
    void shader(const char* name, VkShaderModule& module) {
        std::ifstream input(editorShaderResourcePathCandidate(name), std::ios::binary | std::ios::ate);
        if (!input) throw std::runtime_error("viewport output shader unavailable");
        const auto bytes = input.tellg();
        if (bytes < 20 || bytes > 1048576 || bytes % 4)
            throw std::runtime_error("viewport output shader framing");
        std::vector<uint32_t> words(size_t(bytes) / 4);
        input.seekg(0);
        input.read(reinterpret_cast<char*>(words.data()), bytes);
        if (!input || words.front() != 0x07230203)
            throw std::runtime_error("viewport output shader read/magic");
        VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        info.codeSize = size_t(bytes);
        info.pCode = words.data();
        check(function<PFN_vkCreateShaderModule>("vkCreateShaderModule")(device_, &info, nullptr, &module));
    }

public:
    VulkanViewportOutputCopyCandidate(HMODULE library, VkDevice device, VkPhysicalDevice physical,
            VkImage source, VkImageView sourceView, uint32_t width, uint32_t height)
        : library_(library), device_(device), source_(source), width_(width), height_(height) {
        if (!source || !sourceView || !width || !height)
            throw std::runtime_error("viewport output source/extent unavailable");
        VkPhysicalDeviceProperties properties{};
        function<PFN_vkGetPhysicalDeviceProperties>("vkGetPhysicalDeviceProperties")(physical, &properties);
        if (width > properties.limits.maxFramebufferWidth || height > properties.limits.maxFramebufferHeight ||
            width > properties.limits.maxViewportDimensions[0] || height > properties.limits.maxViewportDimensions[1])
            throw std::runtime_error("viewport output extent exceeds device limits");
        VkFormatProperties unorm{}, srgb{};
        function<PFN_vkGetPhysicalDeviceFormatProperties>("vkGetPhysicalDeviceFormatProperties")(physical, VK_FORMAT_R8G8B8A8_UNORM, &unorm);
        function<PFN_vkGetPhysicalDeviceFormatProperties>("vkGetPhysicalDeviceFormatProperties")(physical, VK_FORMAT_R8G8B8A8_SRGB, &srgb);
        if (!(unorm.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) ||
            !(srgb.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT))
            throw std::runtime_error("viewport output format unsupported");
        try {
            VkImageCreateInfo image{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
            image.flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
            image.imageType = VK_IMAGE_TYPE_2D;
            image.format = VK_FORMAT_R8G8B8A8_UNORM;
            image.extent = {width, height, 1};
            image.mipLevels = image.arrayLayers = 1;
            image.samples = VK_SAMPLE_COUNT_1_BIT;
            image.tiling = VK_IMAGE_TILING_OPTIMAL;
            image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            check(function<PFN_vkCreateImage>("vkCreateImage")(device_, &image, nullptr, &image_));
            VkMemoryRequirements requirements{};
            function<PFN_vkGetImageMemoryRequirements>("vkGetImageMemoryRequirements")(device_, image_, &requirements);
            VkPhysicalDeviceMemoryProperties memory{};
            function<PFN_vkGetPhysicalDeviceMemoryProperties>("vkGetPhysicalDeviceMemoryProperties")(physical, &memory);
            uint32_t type = UINT32_MAX;
            for (uint32_t index = 0; index < memory.memoryTypeCount; ++index)
                if ((requirements.memoryTypeBits & (1u << index)) &&
                    (memory.memoryTypes[index].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) { type = index; break; }
            if (type == UINT32_MAX) throw std::runtime_error("viewport output memory unavailable");
            VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
            allocation.allocationSize = requirements.size;
            allocation.memoryTypeIndex = type;
            check(function<PFN_vkAllocateMemory>("vkAllocateMemory")(device_, &allocation, nullptr, &memory_));
            check(function<PFN_vkBindImageMemory>("vkBindImageMemory")(device_, image_, memory_, 0));
            VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            view.image = image_;
            view.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view.format = VK_FORMAT_R8G8B8A8_SRGB;
            view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            check(function<PFN_vkCreateImageView>("vkCreateImageView")(device_, &view, nullptr, &view_));
            VkAttachmentDescription attachment{};
            attachment.format = VK_FORMAT_R8G8B8A8_SRGB;
            attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            VkAttachmentReference color{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &color;
            VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
            pass.attachmentCount = pass.subpassCount = 1;
            pass.pAttachments = &attachment;
            pass.pSubpasses = &subpass;
            check(function<PFN_vkCreateRenderPass>("vkCreateRenderPass")(device_, &pass, nullptr, &pass_));
            VkFramebufferCreateInfo framebuffer{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
            framebuffer.renderPass = pass_;
            framebuffer.attachmentCount = 1;
            framebuffer.pAttachments = &view_;
            framebuffer.width = width;
            framebuffer.height = height;
            framebuffer.layers = 1;
            check(function<PFN_vkCreateFramebuffer>("vkCreateFramebuffer")(device_, &framebuffer, nullptr, &framebuffer_));
            VkDescriptorSetLayoutBinding binding{0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
            VkDescriptorSetLayoutCreateInfo setLayout{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
            setLayout.bindingCount = 1;
            setLayout.pBindings = &binding;
            check(function<PFN_vkCreateDescriptorSetLayout>("vkCreateDescriptorSetLayout")(device_, &setLayout, nullptr, &descriptorLayout_));
            VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1};
            VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
            pool.maxSets = pool.poolSizeCount = 1;
            pool.pPoolSizes = &size;
            check(function<PFN_vkCreateDescriptorPool>("vkCreateDescriptorPool")(device_, &pool, nullptr, &pool_));
            VkDescriptorSetAllocateInfo set{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            set.descriptorPool = pool_;
            set.descriptorSetCount = 1;
            set.pSetLayouts = &descriptorLayout_;
            check(function<PFN_vkAllocateDescriptorSets>("vkAllocateDescriptorSets")(device_, &set, &descriptor_));
            VkDescriptorImageInfo sampled{VK_NULL_HANDLE, sourceView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            write.dstSet = descriptor_;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            write.pImageInfo = &sampled;
            function<PFN_vkUpdateDescriptorSets>("vkUpdateDescriptorSets")(device_, 1, &write, 0, nullptr);
            VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
            layout.setLayoutCount = 1;
            layout.pSetLayouts = &descriptorLayout_;
            check(function<PFN_vkCreatePipelineLayout>("vkCreatePipelineLayout")(device_, &layout, nullptr, &layout_));
            shader("sfm64-viewport-copy-vertex.spv", shaders_[0]);
            shader("sfm64-viewport-copy-fragment.spv", shaders_[1]);
            std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
            for (size_t index = 0; index < stages.size(); ++index) {
                stages[index].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                stages[index].stage = index ? VK_SHADER_STAGE_FRAGMENT_BIT : VK_SHADER_STAGE_VERTEX_BIT;
                stages[index].module = shaders_[index];
                stages[index].pName = "main";
            }
            VkPipelineVertexInputStateCreateInfo vertices{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
            VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
            assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
            viewport.viewportCount = viewport.scissorCount = 1;
            VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
            raster.polygonMode = VK_POLYGON_MODE_FILL;
            raster.lineWidth = 1;
            VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
            samples.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            VkPipelineColorBlendAttachmentState channelWrite{};
            channelWrite.colorWriteMask = 15;
            VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
            blend.attachmentCount = 1;
            blend.pAttachments = &channelWrite;
            const VkDynamicState states[]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
            VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
            dynamic.dynamicStateCount = 2;
            dynamic.pDynamicStates = states;
            VkGraphicsPipelineCreateInfo pipeline{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
            pipeline.stageCount = 2;
            pipeline.pStages = stages.data();
            pipeline.pVertexInputState = &vertices;
            pipeline.pInputAssemblyState = &assembly;
            pipeline.pViewportState = &viewport;
            pipeline.pRasterizationState = &raster;
            pipeline.pMultisampleState = &samples;
            pipeline.pColorBlendState = &blend;
            pipeline.pDynamicState = &dynamic;
            pipeline.layout = layout_;
            pipeline.renderPass = pass_;
            check(function<PFN_vkCreateGraphicsPipelines>("vkCreateGraphicsPipelines")(
                device_, VK_NULL_HANDLE, 1, &pipeline, nullptr, &pipeline_));
        } catch (...) { reset(); throw; }
    }
    ~VulkanViewportOutputCopyCandidate() { reset(); }
    VulkanViewportOutputCopyCandidate(const VulkanViewportOutputCopyCandidate&) = delete;
    VulkanViewportOutputCopyCandidate& operator=(const VulkanViewportOutputCopyCandidate&) = delete;

    VkImage record(VkCommandBuffer command) {
        // The scene pass just produced source_ in TRANSFER_SRC layout. Returning
        // an UNORM image prevents a second conversion in the presenter's blit.
        std::array<VkImageMemoryBarrier, 2> barriers{};
        for (auto& barrier : barriers) {
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        }
        barriers[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barriers[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barriers[0].image = source_;
        barriers[1].srcAccessMask = recorded_ ? VK_ACCESS_TRANSFER_READ_BIT : 0;
        barriers[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barriers[1].oldLayout = recorded_ ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
        barriers[1].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barriers[1].image = image_;
        function<PFN_vkCmdPipelineBarrier>("vkCmdPipelineBarrier")(command,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0, 0, nullptr, 0, nullptr, 2, barriers.data());
        VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        pass.renderPass = pass_;
        pass.framebuffer = framebuffer_;
        pass.renderArea = {{0, 0}, {width_, height_}};
        function<PFN_vkCmdBeginRenderPass>("vkCmdBeginRenderPass")(command, &pass, VK_SUBPASS_CONTENTS_INLINE);
        function<PFN_vkCmdBindPipeline>("vkCmdBindPipeline")(command, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
        VkViewport viewport{0, 0, float(width_), float(height_), 0, 1};
        VkRect2D scissor{{0, 0}, {width_, height_}};
        function<PFN_vkCmdSetViewport>("vkCmdSetViewport")(command, 0, 1, &viewport);
        function<PFN_vkCmdSetScissor>("vkCmdSetScissor")(command, 0, 1, &scissor);
        function<PFN_vkCmdBindDescriptorSets>("vkCmdBindDescriptorSets")(
            command, VK_PIPELINE_BIND_POINT_GRAPHICS, layout_, 0, 1, &descriptor_, 0, nullptr);
        function<PFN_vkCmdDraw>("vkCmdDraw")(command, 3, 1, 0, 0);
        function<PFN_vkCmdEndRenderPass>("vkCmdEndRenderPass")(command);
        auto& output = barriers[1];
        output.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        output.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        output.oldLayout = output.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        function<PFN_vkCmdPipelineBarrier>("vkCmdPipelineBarrier")(command,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &output);
        recorded_ = true;
        return image_;
    }
};
