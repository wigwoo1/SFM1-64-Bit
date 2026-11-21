#pragma once
#include "editor_shader_resources_candidate.h"
#include "vulkan_offscreen_target_candidate.h"
#include "vulkan_scene_vertex_input_candidate.h"
#include "vulkan_scene_commands_candidate.h"
#include "render_camera_candidate.h"
#include <fstream>
#include <vector>

struct AmbientOcclusionFrontDrawCandidate {
    uint32_t firstIndex, indexCount;
    VkDescriptorSet material;
    MaterialUvTransformCandidate uvTransform;
    float alphaCutoff;
};

// Owned geometric normal and linear depth target. Its resources outlive the
// submitted frame; resize is called only after the previous fence completes.
class VulkanAmbientOcclusionFrontCandidate {
    HMODULE library_;
    VkDevice device_;
    VkPhysicalDevice physical_;
    VkRenderPass pass_ = VK_NULL_HANDLE;
    VkPipelineLayout layout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    std::unique_ptr<VulkanOffscreenTargetCandidate> target_;

    template<class F> F proc(const char* name) const { return load<F>(library_, name); }
    static void check(VkResult result) {
        if (result != VK_SUCCESS)
            throw std::runtime_error("AO front Vulkan result " + std::to_string(result));
    }
    void reset() noexcept {
        target_.reset();
        if (pipeline_) proc<PFN_vkDestroyPipeline>("vkDestroyPipeline")(device_, pipeline_, nullptr);
        if (layout_) proc<PFN_vkDestroyPipelineLayout>("vkDestroyPipelineLayout")(device_, layout_, nullptr);
        if (pass_) proc<PFN_vkDestroyRenderPass>("vkDestroyRenderPass")(device_, pass_, nullptr);
    }
    VkShaderModule loadShader(const char* path) const {
        std::ifstream file(editorShaderResourcePathCandidate(path), std::ios::binary | std::ios::ate);
        const auto size = static_cast<std::streamoff>(file.tellg());
        if (size < 20 || size % 4) throw std::runtime_error("AO front shader size");
        std::vector<uint32_t> words(static_cast<size_t>(size) / 4);
        file.seekg(0);
        if (!file.read(reinterpret_cast<char*>(words.data()), size) || words.front() != 0x07230203)
            throw std::runtime_error("AO front SPIR-V file");
        VkShaderModuleCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        info.codeSize = static_cast<size_t>(size);
        info.pCode = words.data();
        VkShaderModule module{};
        check(proc<PFN_vkCreateShaderModule>("vkCreateShaderModule")(device_, &info, nullptr, &module));
        return module;
    }
    void createPass() {
        std::array<VkAttachmentDescription, 2> attachments{};
        attachments[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attachments[1].format = VK_FORMAT_D32_SFLOAT;
        for (auto& attachment : attachments) {
            attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        }
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        const VkAttachmentReference color{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        const VkAttachmentReference depth{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color;
        subpass.pDepthStencilAttachment = &depth;
        VkRenderPassCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.attachmentCount = static_cast<uint32_t>(attachments.size());
        info.pAttachments = attachments.data();
        info.subpassCount = 1;
        info.pSubpasses = &subpass;
        check(proc<PFN_vkCreateRenderPass>("vkCreateRenderPass")(device_, &info, nullptr, &pass_));
    }
    void createPipeline(VkShaderModule vertex, VkShaderModule fragment) {
        VkPipelineShaderStageCreateInfo stages[2]{};
        for (auto& stage : stages) {
            stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stage.pName = "main";
        }
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertex;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragment;
        VkVertexInputBindingDescription bindings[]{vulkanSceneVertexBindingCandidate(),
                                                    {1, 12, VK_VERTEX_INPUT_RATE_VERTEX}};
        auto sceneAttributes = vulkanSceneVertexAttributesCandidate();
        VkVertexInputAttributeDescription attributes[]{sceneAttributes[0], sceneAttributes[1],
                                                        {1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0}};
        VkPipelineVertexInputStateCreateInfo input{};
        input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        input.vertexBindingDescriptionCount = 2;
        input.pVertexBindingDescriptions = bindings;
        input.vertexAttributeDescriptionCount = 3;
        input.pVertexAttributeDescriptions = attributes;
        VkPipelineInputAssemblyStateCreateInfo assembly{};
        assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{};
        viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport.scissorCount = viewport.viewportCount = 1;
        VkPipelineRasterizationStateCreateInfo raster{};
        raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        raster.cullMode = VK_CULL_MODE_NONE;
        raster.lineWidth = 1;
        VkPipelineMultisampleStateCreateInfo sampling{};
        sampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        sampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo depth{};
        depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth.depthTestEnable = depth.depthWriteEnable = VK_TRUE;
        depth.depthCompareOp = VK_COMPARE_OP_LESS;
        VkPipelineColorBlendAttachmentState output{};
        output.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo color{};
        color.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color.attachmentCount = 1;
        color.pAttachments = &output;
        VkDynamicState states[]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{};
        dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic.dynamicStateCount = 2;
        dynamic.pDynamicStates = states;
        VkGraphicsPipelineCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        info.stageCount = 2;
        info.pStages = stages;
        info.pVertexInputState = &input;
        info.pInputAssemblyState = &assembly;
        info.pViewportState = &viewport;
        info.pRasterizationState = &raster;
        info.pMultisampleState = &sampling;
        info.pDepthStencilState = &depth;
        info.pColorBlendState = &color;
        info.pDynamicState = &dynamic;
        info.layout = layout_;
        info.renderPass = pass_;
        check(proc<PFN_vkCreateGraphicsPipelines>("vkCreateGraphicsPipelines")(
            device_, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline_));
    }
public:
    VulkanAmbientOcclusionFrontCandidate(HMODULE library, VkDevice device, VkPhysicalDevice physical,
                                         VkDescriptorSetLayout material)
        : library_(library), device_(device), physical_(physical) {
        VkShaderModule vertex{}, fragment{};
        const auto destroyShader = proc<PFN_vkDestroyShaderModule>("vkDestroyShaderModule");
        try {
            createPass();
            const VkPushConstantRange push[]{
                {VK_SHADER_STAGE_VERTEX_BIT, 0, 64}, {VK_SHADER_STAGE_FRAGMENT_BIT, 64, 36},
            };
            VkPipelineLayoutCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            info.setLayoutCount = 1;
            info.pSetLayouts = &material;
            info.pushConstantRangeCount = 2;
            info.pPushConstantRanges = push;
            check(proc<PFN_vkCreatePipelineLayout>("vkCreatePipelineLayout")(device_, &info, nullptr, &layout_));
            vertex = loadShader("build/session-core/sfm64-ao-front-vertex-candidate.spv");
            fragment = loadShader("build/session-core/sfm64-ao-front-fragment-candidate.spv");
            createPipeline(vertex, fragment);
        } catch (...) {
            if (fragment) destroyShader(device_, fragment, nullptr);
            if (vertex) destroyShader(device_, vertex, nullptr);
            reset();
            throw;
        }
        destroyShader(device_, fragment, nullptr);
        destroyShader(device_, vertex, nullptr);
    }
    VulkanAmbientOcclusionFrontCandidate(const VulkanAmbientOcclusionFrontCandidate&) = delete;
    ~VulkanAmbientOcclusionFrontCandidate() { reset(); }

    void resize(uint32_t width, uint32_t height) {
        if (target_ && target_->width == width && target_->height == height) return;
        target_ = std::make_unique<VulkanOffscreenTargetCandidate>(
            library_, device_, physical_, pass_, width, height, VK_FORMAT_R32G32B32A32_SFLOAT);
    }
    VkImageView view() const { return target_ ? target_->colorView : VK_NULL_HANDLE; }

    void record(VkCommandBuffer command, const VulkanSceneCommandFunctionsCandidate& fn,
                const RenderCameraCandidate& camera, VkBuffer vertices, VkBuffer normals, VkBuffer indices,
                const std::vector<AmbientOcclusionFrontDrawCandidate>& draws, VkBuffer readback,
                bool sampleMask = false) const {
        if (!target_ || (!draws.empty() && (!vertices || !normals || !indices)))
            throw std::runtime_error("AO front target or geometry missing");
        VkClearValue clear[2]{};
        clear[1].depthStencil.depth = 1;
        VkRenderPassBeginInfo render{};
        render.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render.renderPass = pass_;
        render.framebuffer = target_->framebuffer;
        render.renderArea = {{0, 0}, {target_->width, target_->height}};
        render.clearValueCount = 2;
        render.pClearValues = clear;
        proc<PFN_vkCmdBeginRenderPass>("vkCmdBeginRenderPass")(command, &render, VK_SUBPASS_CONTENTS_INLINE);
        const VkViewport viewport{0, 0, static_cast<float>(target_->width), static_cast<float>(target_->height), 0, 1};
        proc<PFN_vkCmdSetViewport>("vkCmdSetViewport")(command, 0, 1, &viewport);
        proc<PFN_vkCmdSetScissor>("vkCmdSetScissor")(command, 0, 1, &render.renderArea);
        if (!draws.empty()) {
            const VkBuffer streams[]{vertices, normals};
            const VkDeviceSize offsets[]{0, 0};
            fn.pipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
            fn.vertices(command, 0, 2, streams, offsets);
            fn.indices(command, indices, 0, VK_INDEX_TYPE_UINT32);
            const auto push = proc<PFN_vkCmdPushConstants>("vkCmdPushConstants");
            push(command, layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(camera), &camera);
            for (const auto& draw : draws) {
                fn.descriptors(command, VK_PIPELINE_BIND_POINT_GRAPHICS, layout_, 0, 1, &draw.material, 0, nullptr);
                push(command, layout_, VK_SHADER_STAGE_FRAGMENT_BIT, 64, 32, draw.uvTransform.data());
                push(command, layout_, VK_SHADER_STAGE_FRAGMENT_BIT, 96, 4, &draw.alphaCutoff);
                fn.draw(command, draw.indexCount, 1, draw.firstIndex, 0, 0);
            }
        }
        proc<PFN_vkCmdEndRenderPass>("vkCmdEndRenderPass")(command);
        VkImageMemoryBarrier ready{};
        ready.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        ready.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        ready.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        ready.oldLayout = ready.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        ready.srcQueueFamilyIndex = ready.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        ready.image = target_->color;
        ready.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        const auto barrier = proc<PFN_vkCmdPipelineBarrier>("vkCmdPipelineBarrier");
        if (readback) {
            barrier(command, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0, 0, nullptr, 0, nullptr, 1, &ready);
            VkBufferImageCopy copy{};
            copy.imageExtent = {target_->width, target_->height, 1};
            copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
            proc<PFN_vkCmdCopyImageToBuffer>("vkCmdCopyImageToBuffer")(
                command, target_->color, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback, 1, &copy);
            VkMemoryBarrier host{};
            host.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            host.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            host.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
            barrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                    0, 1, &host, 0, nullptr, 0, nullptr);
        }
        if (sampleMask) {
            ready.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            ready.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            VkPipelineStageFlags source = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            if (readback) {
                source |= VK_PIPELINE_STAGE_TRANSFER_BIT;
                ready.srcAccessMask |= VK_ACCESS_TRANSFER_READ_BIT;
            }
            barrier(command, source, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    0, 0, nullptr, 0, nullptr, 1, &ready);
        }
    }
};
