// Reuse the existing owned Vulkan context and allocation instrumentation.
#define main priorTexturePlanProbeMain
#include "probe_vulkan_texture_plan_x64.cpp"
#undef main
#include "vulkan_offscreen_target_candidate.h"
#include "vulkan_viewport_output_copy_candidate.h"
#include <functional>
#include <memory>

struct BlendInput {
    std::array<float, 4> color;
    uint32_t mode;
};
static_assert(sizeof(BlendInput) == 20, "test push layout");

class BlendOutputProbe {
    Context& context_;
    std::vector<std::function<void()>> cleanup_;
    VkRenderPass pass_ = VK_NULL_HANDLE;
    VkPipelineLayout layout_ = VK_NULL_HANDLE;
    std::array<VkPipeline, 3> pipelines_{};
    VkCommandPool pool_ = VK_NULL_HANDLE;
    VkCommandBuffer command_ = VK_NULL_HANDLE;
    VkFence fence_ = VK_NULL_HANDLE;
    std::unique_ptr<VulkanOffscreenTargetCandidate> target_;
    std::unique_ptr<VulkanViewportOutputCopyCandidate> output_;
    VkBuffer capture_ = VK_NULL_HANDLE;
    VkDeviceMemory captureMemory_ = VK_NULL_HANDLE;
    VkBuffer floatCapture_ = VK_NULL_HANDLE;
    VkDeviceMemory floatMemory_ = VK_NULL_HANDLE;
    bool pending_ = false;
    static constexpr uint32_t width = 256, height = 6;

    void reset() noexcept {
        if (pending_) raw<PFN_vkDeviceWaitIdle>(context_.library, "vkDeviceWaitIdle")(context_.device);
        output_.reset();
        target_.reset();
        for (auto iterator = cleanup_.rbegin(); iterator != cleanup_.rend(); ++iterator) (*iterator)();
        cleanup_.clear();
    }
    VkShaderModule shader(const std::filesystem::path& path) {
        std::ifstream input(path, std::ios::binary | std::ios::ate);
        need(bool(input), "blend test shader unavailable");
        const auto size = input.tellg();
        need(size >= 20 && size % 4 == 0, "blend test shader framing");
        std::vector<uint32_t> words(size_t(size) / 4);
        input.seekg(0); input.read(reinterpret_cast<char*>(words.data()), size);
        need(bool(input) && words[0] == 0x07230203, "blend test shader magic");
        VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        info.codeSize = size_t(size); info.pCode = words.data();
        VkShaderModule module{};
        check(raw<PFN_vkCreateShaderModule>(context_.library, "vkCreateShaderModule")(context_.device, &info, nullptr, &module));
        cleanup_.push_back([this, module] { raw<PFN_vkDestroyShaderModule>(context_.library, "vkDestroyShaderModule")(context_.device, module, nullptr); });
        return module;
    }
    void buffer(VkDeviceSize size, VkBuffer& result, VkDeviceMemory& memory) {
        VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        info.size = size; info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        check(load<PFN_vkCreateBuffer>(context_.library, "vkCreateBuffer")(context_.device, &info, nullptr, &result));
        cleanup_.push_back([this, value = result] { load<PFN_vkDestroyBuffer>(context_.library, "vkDestroyBuffer")(context_.device, value, nullptr); });
        VkMemoryRequirements requirements{};
        raw<PFN_vkGetBufferMemoryRequirements>(context_.library, "vkGetBufferMemoryRequirements")(context_.device, result, &requirements);
        context_.allocate(requirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, memory);
        // Free after destroying the bound buffer (cleanup runs in reverse).
        cleanup_.insert(cleanup_.end() - 1, [this, value = memory] { load<PFN_vkFreeMemory>(context_.library, "vkFreeMemory")(context_.device, value, nullptr); });
        check(load<PFN_vkBindBufferMemory>(context_.library, "vkBindBufferMemory")(context_.device, result, memory, 0));
    }
    void draw(uint32_t row, uint32_t pipeline, BlendInput input) {
        VkRect2D scissor{{0, int32_t(row)}, {width, 1}};
        raw<PFN_vkCmdSetScissor>(context_.library, "vkCmdSetScissor")(command_, 0, 1, &scissor);
        raw<PFN_vkCmdBindPipeline>(context_.library, "vkCmdBindPipeline")(command_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines_.at(pipeline));
        raw<PFN_vkCmdPushConstants>(context_.library, "vkCmdPushConstants")(command_, layout_, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(input), &input);
        raw<PFN_vkCmdDraw>(context_.library, "vkCmdDraw")(command_, 3, 1, 0, 0);
    }
    void copy(VkImage image, VkBuffer buffer) {
        VkBufferImageCopy region{};
        region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.imageExtent = {width, height, 1};
        raw<PFN_vkCmdCopyImageToBuffer>(context_.library, "vkCmdCopyImageToBuffer")(
            command_, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);
    }
    std::vector<uint8_t> mapped(VkDeviceMemory memory, size_t size) {
        std::vector<uint8_t> bytes(size); void* source = nullptr;
        check(load<PFN_vkMapMemory>(context_.library, "vkMapMemory")(context_.device, memory, 0, size, 0, &source));
        std::memcpy(bytes.data(), source, size);
        load<PFN_vkUnmapMemory>(context_.library, "vkUnmapMemory")(context_.device, memory);
        return bytes;
    }

public:
    BlendOutputProbe(Context& context, const std::filesystem::path& shaders,
                     const std::filesystem::path& testShader) : context_(context) {
        try {
            VkAttachmentDescription attachments[2]{};
            attachments[0].format = VK_FORMAT_R16G16B16A16_SFLOAT;
            attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
            attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[0].finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            attachments[1] = attachments[0];
            attachments[1].format = VK_FORMAT_D32_SFLOAT;
            attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            VkAttachmentReference color{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}, depth{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1; subpass.pColorAttachments = &color; subpass.pDepthStencilAttachment = &depth;
            VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
            pass.attachmentCount = 2; pass.pAttachments = attachments; pass.subpassCount = 1; pass.pSubpasses = &subpass;
            check(raw<PFN_vkCreateRenderPass>(context_.library, "vkCreateRenderPass")(context_.device, &pass, nullptr, &pass_));
            cleanup_.push_back([this] { raw<PFN_vkDestroyRenderPass>(context_.library, "vkDestroyRenderPass")(context_.device, pass_, nullptr); });
            target_ = std::make_unique<VulkanOffscreenTargetCandidate>(context_.library, context_.device,
                context_.physical, pass_, width, height, VK_FORMAT_R16G16B16A16_SFLOAT);
            VkPushConstantRange push{VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BlendInput)};
            VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
            layout.pushConstantRangeCount = 1; layout.pPushConstantRanges = &push;
            check(raw<PFN_vkCreatePipelineLayout>(context_.library, "vkCreatePipelineLayout")(context_.device, &layout, nullptr, &layout_));
            cleanup_.push_back([this] { raw<PFN_vkDestroyPipelineLayout>(context_.library, "vkDestroyPipelineLayout")(context_.device, layout_, nullptr); });
            std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
            stages[0].sType = stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT; stages[0].module = shader(shaders / "sfm64-viewport-copy-vertex.spv");
            stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT; stages[1].module = shader(testShader);
            stages[0].pName = stages[1].pName = "main";
            VkPipelineVertexInputStateCreateInfo vertices{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
            VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
            assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
            viewport.viewportCount = viewport.scissorCount = 1;
            VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
            raster.polygonMode = VK_POLYGON_MODE_FILL; raster.lineWidth = 1;
            VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
            samples.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            VkPipelineDepthStencilStateCreateInfo depthState{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
            VkPipelineColorBlendAttachmentState write{}; write.colorWriteMask = 15;
            VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
            blend.attachmentCount = 1; blend.pAttachments = &write;
            VkDynamicState states[]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
            VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
            dynamic.dynamicStateCount = 2; dynamic.pDynamicStates = states;
            VkGraphicsPipelineCreateInfo pipeline{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
            pipeline.stageCount = 2; pipeline.pStages = stages.data(); pipeline.pVertexInputState = &vertices;
            pipeline.pInputAssemblyState = &assembly; pipeline.pViewportState = &viewport; pipeline.pRasterizationState = &raster;
            pipeline.pMultisampleState = &samples; pipeline.pDepthStencilState = &depthState;
            pipeline.pColorBlendState = &blend; pipeline.pDynamicState = &dynamic; pipeline.layout = layout_; pipeline.renderPass = pass_;
            for (uint32_t mode = 0; mode < 3; ++mode) {
                write.blendEnable = mode != 0;
                write.srcColorBlendFactor = mode == 2 ? VK_BLEND_FACTOR_ONE : VK_BLEND_FACTOR_SRC_ALPHA;
                write.dstColorBlendFactor = mode == 2 ? VK_BLEND_FACTOR_ONE : VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                write.colorBlendOp = write.alphaBlendOp = VK_BLEND_OP_ADD;
                write.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                write.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                write.colorWriteMask = mode == 2 ? 7u : 15u;
                check(raw<PFN_vkCreateGraphicsPipelines>(context_.library, "vkCreateGraphicsPipelines")(
                    context_.device, VK_NULL_HANDLE, 1, &pipeline, nullptr, &pipelines_[mode]));
                cleanup_.push_back([this, value = pipelines_[mode]] { raw<PFN_vkDestroyPipeline>(context_.library, "vkDestroyPipeline")(context_.device, value, nullptr); });
            }
            VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
            pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; pool.queueFamilyIndex = context_.family;
            check(raw<PFN_vkCreateCommandPool>(context_.library, "vkCreateCommandPool")(context_.device, &pool, nullptr, &pool_));
            cleanup_.push_back([this] { raw<PFN_vkDestroyCommandPool>(context_.library, "vkDestroyCommandPool")(context_.device, pool_, nullptr); });
            VkCommandBufferAllocateInfo allocation{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
            allocation.commandPool = pool_; allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; allocation.commandBufferCount = 1;
            check(raw<PFN_vkAllocateCommandBuffers>(context_.library, "vkAllocateCommandBuffers")(context_.device, &allocation, &command_));
            VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
            check(raw<PFN_vkCreateFence>(context_.library, "vkCreateFence")(context_.device, &fence, nullptr, &fence_));
            cleanup_.push_back([this] { raw<PFN_vkDestroyFence>(context_.library, "vkDestroyFence")(context_.device, fence_, nullptr); });
            buffer(width * height * 4, capture_, captureMemory_);
            buffer(width * height * 8, floatCapture_, floatMemory_);
            output_ = std::make_unique<VulkanViewportOutputCopyCandidate>(context_.library, context_.device,
                context_.physical, target_->color, target_->colorView, width, height);
        } catch (...) { reset(); throw; }
    }
    ~BlendOutputProbe() { reset(); }
    BlendOutputProbe(const BlendOutputProbe&) = delete;
    BlendOutputProbe& operator=(const BlendOutputProbe&) = delete;

    std::pair<std::vector<uint8_t>, std::vector<uint8_t>> render(bool blank = false) {
        check(raw<PFN_vkResetCommandBuffer>(context_.library, "vkResetCommandBuffer")(command_, 0));
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        check(raw<PFN_vkBeginCommandBuffer>(context_.library, "vkBeginCommandBuffer")(command_, &begin));
        VkMemoryBarrier reuse{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        reuse.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        reuse.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        raw<PFN_vkCmdPipelineBarrier>(context_.library, "vkCmdPipelineBarrier")(command_, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 1, &reuse, 0, nullptr, 0, nullptr);
        VkClearValue clears[2]{}; clears[1].depthStencil.depth = 1;
        VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        pass.renderPass = pass_; pass.framebuffer = target_->framebuffer;
        pass.renderArea = {{0, 0}, {width, height}}; pass.clearValueCount = 2; pass.pClearValues = clears;
        raw<PFN_vkCmdBeginRenderPass>(context_.library, "vkCmdBeginRenderPass")(command_, &pass, VK_SUBPASS_CONTENTS_INLINE);
        VkViewport viewport{0, 0, float(width), float(height), 0, 1};
        raw<PFN_vkCmdSetViewport>(context_.library, "vkCmdSetViewport")(command_, 0, 1, &viewport);
        if (!blank) {
            draw(0, 0, {{.18f, .5f, .003f, 1}, 1});
            draw(1, 0, {{.1f, .6f, .001f, .25f}, 0}); draw(1, 1, {{.8f, .2f, .5f, 1}, 1});
            draw(2, 0, {{.125f, .5f, .9f, .125f}, 0});
            draw(2, 1, {{3, -.25f, .5f, .5f}, 0}); draw(2, 1, {{.001f, .18f, .25f, 1}, 1});
            draw(3, 0, {{.25f, .5f, 2, .15f}, 0});
            draw(3, 2, {{1.5f, .25f, -.125f, 0}, 0}); draw(3, 1, {{.001f, .18f, .25f, 1}, 1});
            draw(4, 0, {{0, 0, 0, 0}, 2});
            draw(5, 0, {{.0031308f, .003f, .0032f, 1}, 1});
        }
        raw<PFN_vkCmdEndRenderPass>(context_.library, "vkCmdEndRenderPass")(command_);
        const auto image = output_->record(command_);
        copy(image, capture_);
        VkImageMemoryBarrier floatRead{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        floatRead.srcAccessMask = VK_ACCESS_SHADER_READ_BIT; floatRead.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        floatRead.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; floatRead.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        floatRead.srcQueueFamilyIndex = floatRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        floatRead.image = target_->color; floatRead.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        raw<PFN_vkCmdPipelineBarrier>(context_.library, "vkCmdPipelineBarrier")(command_, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &floatRead);
        copy(target_->color, floatCapture_);
        VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        host.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; host.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        raw<PFN_vkCmdPipelineBarrier>(context_.library, "vkCmdPipelineBarrier")(command_, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &host, 0, nullptr, 0, nullptr);
        check(raw<PFN_vkEndCommandBuffer>(context_.library, "vkEndCommandBuffer")(command_));
        check(raw<PFN_vkResetFences>(context_.library, "vkResetFences")(context_.device, 1, &fence_));
        VkSubmitInfo submission{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submission.commandBufferCount = 1; submission.pCommandBuffers = &command_;
        check(raw<PFN_vkQueueSubmit>(context_.library, "vkQueueSubmit")(context_.queue, 1, &submission, fence_)); pending_ = true;
        check(raw<PFN_vkWaitForFences>(context_.library, "vkWaitForFences")(context_.device, 1, &fence_, VK_TRUE, 10000000000ull)); pending_ = false;
        return {mapped(captureMemory_, width * height * 4), mapped(floatMemory_, width * height * 8)};
    }
};

int main(int argc, char** argv) {
    try {
        static_assert(sizeof(void*) == 8, "native x64 GPU test");
        need(argc == 4, "editor shaders test shader and fresh output directory required");
        const std::filesystem::path out = argv[3];
        need(std::filesystem::create_directory(out), "fresh output directory required");
        editorShaderDirectoryCandidate = std::filesystem::path(argv[1]);
        Context context;
        {
            BlendOutputProbe probe(context, argv[1], argv[2]);
            std::pair<std::vector<uint8_t>, std::vector<uint8_t>> previous;
            for (int frame = 0; frame < 4; ++frame) {
                auto pixels = probe.render();
                if (frame) need(pixels == previous, "repeated blending/copy changed pixels");
                previous = pixels;
            }
            auto save = [&](const char* name, const auto& bytes) { std::ofstream file(out / name, std::ios::binary); file.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size())); need(bool(file), "capture write"); };
            save("encoded.rgba", previous.first); save("linear.rgba16f", previous.second);
            auto blank = probe.render(true);
            need(std::all_of(blank.first.begin(), blank.first.end(), [](uint8_t value) { return value == 0; }), "blank output retained pixels or alpha");
            need(probe.render() == previous, "blank restore changed blending/copy");
        }
        need(live() == std::array<size_t, 5>{} && !tracked.violations, "tracked GPU resources leaked");
        std::cout << "PASS shared_editor_output_copy=1 alpha_columns=256 rows=6 submitted_frames=6 repeated_exact=1 blank_restore=1 tracked_memory_released=1 original_runtime=ABSENT\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
