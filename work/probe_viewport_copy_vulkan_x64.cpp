#define NOMINMAX
#include <windows.h>
#include <vulkan/vulkan.h>
#include <atomic>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include "viewport_copy_fixture_candidate.h"

// Same system-loader dispatch convention as the editor and existing GPU probes.
template<class Function> static Function load(HMODULE library, const char* name) {
    auto address = GetProcAddress(library, name);
    if (!address) throw std::runtime_error(name);
    return reinterpret_cast<Function>(address);
}

static void check(VkResult result) {
    if (result != VK_SUCCESS) throw std::runtime_error("Vulkan result " + std::to_string(result));
}

struct Buffer {
    VkBuffer handle = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
};
struct Image {
    VkImage handle = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    std::array<VkImageView, 2> views{};
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
};

class CopyFixture {
    HMODULE library_ = nullptr;
    VkInstance instance_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT messenger_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue queue_ = VK_NULL_HANDLE;
    uint32_t family_ = 0;
    VkPhysicalDeviceMemoryProperties memoryProperties_{};
    VkCommandPool commands_ = VK_NULL_HANDLE;
    VkCommandBuffer command_ = VK_NULL_HANDLE;
    VkFence fence_ = VK_NULL_HANDLE;
    bool pending_ = false;
    VkDescriptorSetLayout setLayout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptors_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    std::vector<VkShaderModule> modules_;
    std::vector<VkRenderPass> passes_;
    std::vector<VkPipeline> pipelines_;
    std::vector<VkFramebuffer> framebuffers_;
    std::deque<Buffer> buffers_;
    std::deque<Image> images_;
    std::atomic<unsigned> validationErrors_{0};
    bool validationEnabled_ = false;
    uint32_t width_ = 0;

    static VKAPI_ATTR VkBool32 VKAPI_CALL debug(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
            VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* data, void* owner) {
        if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            ++static_cast<CopyFixture*>(owner)->validationErrors_;
        std::cerr << "validation " << data->pMessage << '\n';
        return VK_FALSE;
    }

    void allocate(const VkMemoryRequirements& requirements, VkMemoryPropertyFlags flags,
                  VkDeviceMemory& memory) {
        uint32_t type = UINT32_MAX;
        for (uint32_t index = 0; index < memoryProperties_.memoryTypeCount; ++index) {
            if ((requirements.memoryTypeBits & (1u << index)) &&
                (memoryProperties_.memoryTypes[index].propertyFlags & flags) == flags) {
                type = index;
                break;
            }
        }
        if (type == UINT32_MAX) throw std::runtime_error("required Vulkan memory unavailable");
        VkMemoryAllocateInfo info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        info.allocationSize = requirements.size;
        info.memoryTypeIndex = type;
        check(load<PFN_vkAllocateMemory>(library_, "vkAllocateMemory")(device_, &info, nullptr, &memory));
    }

    Buffer& buffer(VkDeviceSize size, VkBufferUsageFlags usage) {
        buffers_.emplace_back();
        auto& result = buffers_.back();
        VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        info.size = size;
        info.usage = usage;
        check(load<PFN_vkCreateBuffer>(library_, "vkCreateBuffer")(device_, &info, nullptr, &result.handle));
        VkMemoryRequirements requirements{};
        load<PFN_vkGetBufferMemoryRequirements>(library_, "vkGetBufferMemoryRequirements")(device_, result.handle, &requirements);
        allocate(requirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, result.memory);
        check(load<PFN_vkBindBufferMemory>(library_, "vkBindBufferMemory")(device_, result.handle, result.memory, 0));
        return result;
    }

    Image& image(VkFormat format, VkImageUsageFlags usage, VkFormat alternate = VK_FORMAT_UNDEFINED) {
        const VkImageCreateFlags flags = alternate == VK_FORMAT_UNDEFINED ? 0u : VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
        VkImageFormatProperties supported{};
        check(load<PFN_vkGetPhysicalDeviceImageFormatProperties>(library_, "vkGetPhysicalDeviceImageFormatProperties")(
            physical_, format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, flags, &supported));
        if (width_ > supported.maxExtent.width || !(supported.sampleCounts & VK_SAMPLE_COUNT_1_BIT))
            throw std::runtime_error("copy image format extent/sample count unsupported");
        images_.emplace_back();
        auto& result = images_.back();
        VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        info.flags = flags;
        info.imageType = VK_IMAGE_TYPE_2D;
        info.format = format;
        info.extent = {width_, 1, 1};
        info.mipLevels = info.arrayLayers = 1;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.usage = usage;
        check(load<PFN_vkCreateImage>(library_, "vkCreateImage")(device_, &info, nullptr, &result.handle));
        VkMemoryRequirements requirements{};
        load<PFN_vkGetImageMemoryRequirements>(library_, "vkGetImageMemoryRequirements")(device_, result.handle, &requirements);
        allocate(requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, result.memory);
        check(load<PFN_vkBindImageMemory>(library_, "vkBindImageMemory")(device_, result.handle, result.memory, 0));
        const std::array<VkFormat, 2> formats{format, alternate};
        for (size_t index = 0; index < formats.size(); ++index) {
            if (formats[index] == VK_FORMAT_UNDEFINED) continue;
            VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            view.image = result.handle;
            view.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view.format = formats[index];
            view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            check(load<PFN_vkCreateImageView>(library_, "vkCreateImageView")(device_, &view, nullptr, &result.views[index]));
        }
        return result;
    }

    void begin() {
        if (pending_) throw std::runtime_error("previous GPU submission is still pending");
        check(load<PFN_vkResetCommandBuffer>(library_, "vkResetCommandBuffer")(command_, 0));
        VkCommandBufferBeginInfo info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        check(load<PFN_vkBeginCommandBuffer>(library_, "vkBeginCommandBuffer")(command_, &info));
    }

    void submit() {
        check(load<PFN_vkEndCommandBuffer>(library_, "vkEndCommandBuffer")(command_));
        check(load<PFN_vkResetFences>(library_, "vkResetFences")(device_, 1, &fence_));
        VkSubmitInfo info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        info.commandBufferCount = 1;
        info.pCommandBuffers = &command_;
        check(load<PFN_vkQueueSubmit>(library_, "vkQueueSubmit")(queue_, 1, &info, fence_));
        pending_ = true;
        check(load<PFN_vkWaitForFences>(library_, "vkWaitForFences")(device_, 1, &fence_, VK_TRUE, 10000000000ull));
        pending_ = false;
    }

    void transition(Image& target, VkImageLayout layout) {
        if (target.layout == layout) return;
        auto access = [](VkImageLayout value) -> std::pair<VkPipelineStageFlags, VkAccessFlags> {
            switch (value) {
                case VK_IMAGE_LAYOUT_UNDEFINED: return {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0};
                case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: return {VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT};
                case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: return {VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT};
                case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: return {VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT};
                case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: return {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT};
                default: throw std::runtime_error("unexpected fixture image layout");
            }
        };
        const auto source = access(target.layout), destination = access(layout);
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.srcAccessMask = source.second;
        barrier.dstAccessMask = destination.second;
        barrier.oldLayout = target.layout;
        barrier.newLayout = layout;
        barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = target.handle;
        barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        load<PFN_vkCmdPipelineBarrier>(library_, "vkCmdPipelineBarrier")(
            command_, source.first, destination.first, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        target.layout = layout;
    }

    VkDescriptorSet descriptor(VkImageView view) {
        VkDescriptorSet set = VK_NULL_HANDLE;
        VkDescriptorSetAllocateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        info.descriptorPool = descriptors_;
        info.descriptorSetCount = 1;
        info.pSetLayouts = &setLayout_;
        check(load<PFN_vkAllocateDescriptorSets>(library_, "vkAllocateDescriptorSets")(device_, &info, &set));
        VkDescriptorImageInfo imageInfo{VK_NULL_HANDLE, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = set;
        write.dstBinding = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        write.pImageInfo = &imageInfo;
        load<PFN_vkUpdateDescriptorSets>(library_, "vkUpdateDescriptorSets")(device_, 1, &write, 0, nullptr);
        return set;
    }

    VkShaderModule shader(const std::filesystem::path& path) {
        std::ifstream input(path, std::ios::binary | std::ios::ate);
        if (!input) throw std::runtime_error("copy shader unavailable");
        const auto bytes = input.tellg();
        if (bytes < 20 || bytes > 1048576 || bytes % 4) throw std::runtime_error("copy shader framing");
        std::vector<uint32_t> words(static_cast<size_t>(bytes) / 4);
        input.seekg(0);
        input.read(reinterpret_cast<char*>(words.data()), bytes);
        if (!input || words.front() != 0x07230203) throw std::runtime_error("copy shader read/magic");
        VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        info.codeSize = static_cast<size_t>(bytes);
        info.pCode = words.data();
        modules_.push_back(VK_NULL_HANDLE);
        check(load<PFN_vkCreateShaderModule>(library_, "vkCreateShaderModule")(device_, &info, nullptr, &modules_.back()));
        return modules_.back();
    }

    std::pair<VkRenderPass, VkPipeline> pipeline(VkFormat format) {
        VkAttachmentDescription attachment{};
        attachment.format = format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkAttachmentReference color{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color;
        VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        pass.attachmentCount = 1;
        pass.pAttachments = &attachment;
        pass.subpassCount = 1;
        pass.pSubpasses = &subpass;
        passes_.push_back(VK_NULL_HANDLE);
        check(load<PFN_vkCreateRenderPass>(library_, "vkCreateRenderPass")(device_, &pass, nullptr, &passes_.back()));

        std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
        stages[0].sType = stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = modules_.at(0);
        stages[0].pName = "copyVertex";
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = modules_.at(1);
        stages[1].pName = "copyFragment";
        VkPipelineVertexInputStateCreateInfo vertices{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewport.viewportCount = viewport.scissorCount = 1;
        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        raster.cullMode = VK_CULL_MODE_NONE;
        raster.lineWidth = 1;
        VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        samples.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        VkPipelineColorBlendAttachmentState write{};
        write.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        blend.attachmentCount = 1;
        blend.pAttachments = &write;
        std::array<VkDynamicState, 2> dynamicStates{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamic.dynamicStateCount = 2;
        dynamic.pDynamicStates = dynamicStates.data();
        VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        info.stageCount = 2;
        info.pStages = stages.data();
        info.pVertexInputState = &vertices;
        info.pInputAssemblyState = &assembly;
        info.pViewportState = &viewport;
        info.pRasterizationState = &raster;
        info.pMultisampleState = &samples;
        info.pColorBlendState = &blend;
        info.pDynamicState = &dynamic;
        info.layout = pipelineLayout_;
        info.renderPass = passes_.back();
        pipelines_.push_back(VK_NULL_HANDLE);
        check(load<PFN_vkCreateGraphicsPipelines>(library_, "vkCreateGraphicsPipelines")(
            device_, VK_NULL_HANDLE, 1, &info, nullptr, &pipelines_.back()));
        return {passes_.back(), pipelines_.back()};
    }

    VkFramebuffer framebuffer(VkRenderPass pass, VkImageView view) {
        VkFramebufferCreateInfo info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        info.renderPass = pass;
        info.attachmentCount = 1;
        info.pAttachments = &view;
        info.width = width_;
        info.height = info.layers = 1;
        framebuffers_.push_back(VK_NULL_HANDLE);
        check(load<PFN_vkCreateFramebuffer>(library_, "vkCreateFramebuffer")(device_, &info, nullptr, &framebuffers_.back()));
        return framebuffers_.back();
    }

    std::vector<std::array<uint8_t, 3>> drawCopy(Image& source, VkDescriptorSet set,
            Image& output, VkFramebuffer target, std::pair<VkRenderPass, VkPipeline> drawing,
            Buffer& readback, bool bgra) {
        begin();
        transition(source, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        transition(output, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        VkClearValue clear{};
        clear.color = {{1, 0, 1, 1}};
        VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        pass.renderPass = drawing.first;
        pass.framebuffer = target;
        pass.renderArea = {{0, 0}, {width_, 1}};
        pass.clearValueCount = 1;
        pass.pClearValues = &clear;
        load<PFN_vkCmdBeginRenderPass>(library_, "vkCmdBeginRenderPass")(command_, &pass, VK_SUBPASS_CONTENTS_INLINE);
        load<PFN_vkCmdBindPipeline>(library_, "vkCmdBindPipeline")(command_, VK_PIPELINE_BIND_POINT_GRAPHICS, drawing.second);
        VkViewport viewport{0, 0, static_cast<float>(width_), 1, 0, 1};
        VkRect2D scissor{{0, 0}, {width_, 1}};
        load<PFN_vkCmdSetViewport>(library_, "vkCmdSetViewport")(command_, 0, 1, &viewport);
        load<PFN_vkCmdSetScissor>(library_, "vkCmdSetScissor")(command_, 0, 1, &scissor);
        load<PFN_vkCmdBindDescriptorSets>(library_, "vkCmdBindDescriptorSets")(
            command_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_, 0, 1, &set, 0, nullptr);
        load<PFN_vkCmdDraw>(library_, "vkCmdDraw")(command_, 3, 1, 0, 0);
        load<PFN_vkCmdEndRenderPass>(library_, "vkCmdEndRenderPass")(command_);
        transition(output, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        VkBufferImageCopy copy{};
        copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copy.imageExtent = {width_, 1, 1};
        load<PFN_vkCmdCopyImageToBuffer>(library_, "vkCmdCopyImageToBuffer")(
            command_, output.handle, output.layout, readback.handle, 1, &copy);
        VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        host.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        host.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        load<PFN_vkCmdPipelineBarrier>(library_, "vkCmdPipelineBarrier")(
            command_, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &host, 0, nullptr, 0, nullptr);
        submit();
        std::vector<std::array<uint8_t, 3>> result(width_);
        void* mapped = nullptr;
        check(load<PFN_vkMapMemory>(library_, "vkMapMemory")(device_, readback.memory, 0, width_ * 4, 0, &mapped));
        const auto* bytes = static_cast<const uint8_t*>(mapped);
        const std::array<size_t, 3> order = bgra ? std::array<size_t, 3>{2, 1, 0} : std::array<size_t, 3>{0, 1, 2};
        for (uint32_t index = 0; index < width_; ++index)
            for (size_t channel = 0; channel < 3; ++channel) result[index][channel] = bytes[index * 4 + order[channel]];
        load<PFN_vkUnmapMemory>(library_, "vkUnmapMemory")(device_, readback.memory);
        return result;
    }

public:
    ~CopyFixture() {
        if (device_) {
            if (pending_) load<PFN_vkDeviceWaitIdle>(library_, "vkDeviceWaitIdle")(device_);
            for (auto object : framebuffers_) if (object) load<PFN_vkDestroyFramebuffer>(library_, "vkDestroyFramebuffer")(device_, object, nullptr);
            for (auto object : pipelines_) if (object) load<PFN_vkDestroyPipeline>(library_, "vkDestroyPipeline")(device_, object, nullptr);
            for (auto object : passes_) if (object) load<PFN_vkDestroyRenderPass>(library_, "vkDestroyRenderPass")(device_, object, nullptr);
            if (descriptors_) load<PFN_vkDestroyDescriptorPool>(library_, "vkDestroyDescriptorPool")(device_, descriptors_, nullptr);
            if (pipelineLayout_) load<PFN_vkDestroyPipelineLayout>(library_, "vkDestroyPipelineLayout")(device_, pipelineLayout_, nullptr);
            if (setLayout_) load<PFN_vkDestroyDescriptorSetLayout>(library_, "vkDestroyDescriptorSetLayout")(device_, setLayout_, nullptr);
            for (auto object : modules_) if (object) load<PFN_vkDestroyShaderModule>(library_, "vkDestroyShaderModule")(device_, object, nullptr);
            for (auto& object : images_) {
                for (auto view : object.views) if (view) load<PFN_vkDestroyImageView>(library_, "vkDestroyImageView")(device_, view, nullptr);
                if (object.handle) load<PFN_vkDestroyImage>(library_, "vkDestroyImage")(device_, object.handle, nullptr);
                if (object.memory) load<PFN_vkFreeMemory>(library_, "vkFreeMemory")(device_, object.memory, nullptr);
            }
            for (auto& object : buffers_) {
                if (object.handle) load<PFN_vkDestroyBuffer>(library_, "vkDestroyBuffer")(device_, object.handle, nullptr);
                if (object.memory) load<PFN_vkFreeMemory>(library_, "vkFreeMemory")(device_, object.memory, nullptr);
            }
            if (fence_) load<PFN_vkDestroyFence>(library_, "vkDestroyFence")(device_, fence_, nullptr);
            if (commands_) load<PFN_vkDestroyCommandPool>(library_, "vkDestroyCommandPool")(device_, commands_, nullptr);
            load<PFN_vkDestroyDevice>(library_, "vkDestroyDevice")(device_, nullptr);
        }
        if (messenger_) {
            auto get = load<PFN_vkGetInstanceProcAddr>(library_, "vkGetInstanceProcAddr");
            reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(get(instance_, "vkDestroyDebugUtilsMessengerEXT"))(instance_, messenger_, nullptr);
        }
        if (instance_) load<PFN_vkDestroyInstance>(library_, "vkDestroyInstance")(instance_, nullptr);
        if (library_) FreeLibrary(library_);
    }

    void initialize(uint32_t width, const std::filesystem::path& shaders) {
        width_ = width;
        library_ = LoadLibraryExW(L"vulkan-1.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!library_) throw std::runtime_error("system Vulkan loader unavailable");
        uint32_t count = 0;
        check(load<PFN_vkEnumerateInstanceLayerProperties>(library_, "vkEnumerateInstanceLayerProperties")(&count, nullptr));
        std::vector<VkLayerProperties> layers(count);
        check(load<PFN_vkEnumerateInstanceLayerProperties>(library_, "vkEnumerateInstanceLayerProperties")(&count, layers.data()));
        for (const auto& layer : layers)
            if (!std::strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation")) validationEnabled_ = true;
        check(load<PFN_vkEnumerateInstanceExtensionProperties>(library_, "vkEnumerateInstanceExtensionProperties")(nullptr, &count, nullptr));
        std::vector<VkExtensionProperties> extensions(count);
        check(load<PFN_vkEnumerateInstanceExtensionProperties>(library_, "vkEnumerateInstanceExtensionProperties")(nullptr, &count, extensions.data()));
        bool debugSupported = false;
        for (const auto& extension : extensions)
            if (!std::strcmp(extension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) debugSupported = true;
        validationEnabled_ = validationEnabled_ && debugSupported;
        const char* layer = "VK_LAYER_KHRONOS_validation";
        const char* extension = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        VkDebugUtilsMessengerCreateInfoEXT debugInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugInfo.pfnUserCallback = debug;
        debugInfo.pUserData = this;
        VkApplicationInfo application{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        application.apiVersion = VK_API_VERSION_1_0;
        application.pApplicationName = "SFM64 controlled viewport copy fixture";
        VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        info.pApplicationInfo = &application;
        if (validationEnabled_) {
            info.enabledLayerCount = info.enabledExtensionCount = 1;
            info.ppEnabledLayerNames = &layer;
            info.ppEnabledExtensionNames = &extension;
            info.pNext = &debugInfo;
        }
        check(load<PFN_vkCreateInstance>(library_, "vkCreateInstance")(&info, nullptr, &instance_));
        if (validationEnabled_) {
            auto get = load<PFN_vkGetInstanceProcAddr>(library_, "vkGetInstanceProcAddr");
            check(reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(get(instance_, "vkCreateDebugUtilsMessengerEXT"))(
                instance_, &debugInfo, nullptr, &messenger_));
        }
        check(load<PFN_vkEnumeratePhysicalDevices>(library_, "vkEnumeratePhysicalDevices")(instance_, &count, nullptr));
        std::vector<VkPhysicalDevice> devices(count);
        check(load<PFN_vkEnumeratePhysicalDevices>(library_, "vkEnumeratePhysicalDevices")(instance_, &count, devices.data()));
        for (auto candidate : devices) {
            VkPhysicalDeviceProperties properties{};
            load<PFN_vkGetPhysicalDeviceProperties>(library_, "vkGetPhysicalDeviceProperties")(candidate, &properties);
            if (properties.vendorID == 0x10DE && properties.deviceID == 0x2D18) {
                physical_ = candidate;
                std::cout << "device_name=" << properties.deviceName << " vendor=0x10de device=0x2d18\n";
                break;
            }
        }
        if (!physical_) throw std::runtime_error("benchmark NVIDIA Vulkan device unavailable");
        load<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(library_, "vkGetPhysicalDeviceQueueFamilyProperties")(physical_, &count, nullptr);
        std::vector<VkQueueFamilyProperties> queues(count);
        load<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(library_, "vkGetPhysicalDeviceQueueFamilyProperties")(physical_, &count, queues.data());
        family_ = UINT32_MAX;
        for (uint32_t index = 0; index < count; ++index) {
            if (queues[index].queueCount && (queues[index].queueFlags & VK_QUEUE_GRAPHICS_BIT)) { family_ = index; break; }
        }
        if (family_ == UINT32_MAX) throw std::runtime_error("graphics queue unavailable");
        const float priority = 1;
        VkDeviceQueueCreateInfo queue{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queue.queueFamilyIndex = family_;
        queue.queueCount = 1;
        queue.pQueuePriorities = &priority;
        VkDeviceCreateInfo device{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        device.queueCreateInfoCount = 1;
        device.pQueueCreateInfos = &queue;
        check(load<PFN_vkCreateDevice>(library_, "vkCreateDevice")(physical_, &device, nullptr, &device_));
        load<PFN_vkGetDeviceQueue>(library_, "vkGetDeviceQueue")(device_, family_, 0, &queue_);
        load<PFN_vkGetPhysicalDeviceMemoryProperties>(library_, "vkGetPhysicalDeviceMemoryProperties")(physical_, &memoryProperties_);
        VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool.queueFamilyIndex = family_;
        check(load<PFN_vkCreateCommandPool>(library_, "vkCreateCommandPool")(device_, &pool, nullptr, &commands_));
        VkCommandBufferAllocateInfo command{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        command.commandPool = commands_;
        command.commandBufferCount = 1;
        command.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        check(load<PFN_vkAllocateCommandBuffers>(library_, "vkAllocateCommandBuffers")(device_, &command, &command_));
        VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        check(load<PFN_vkCreateFence>(library_, "vkCreateFence")(device_, &fence, nullptr, &fence_));
        VkDescriptorSetLayoutBinding binding{0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
        VkDescriptorSetLayoutCreateInfo layout{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        layout.bindingCount = 1;
        layout.pBindings = &binding;
        check(load<PFN_vkCreateDescriptorSetLayout>(library_, "vkCreateDescriptorSetLayout")(device_, &layout, nullptr, &setLayout_));
        VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 8};
        VkDescriptorPoolCreateInfo descriptorPool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        descriptorPool.maxSets = 8;
        descriptorPool.poolSizeCount = 1;
        descriptorPool.pPoolSizes = &size;
        check(load<PFN_vkCreateDescriptorPool>(library_, "vkCreateDescriptorPool")(device_, &descriptorPool, nullptr, &descriptors_));
        VkPipelineLayoutCreateInfo pipelineLayout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        pipelineLayout.setLayoutCount = 1;
        pipelineLayout.pSetLayouts = &setLayout_;
        check(load<PFN_vkCreatePipelineLayout>(library_, "vkCreatePipelineLayout")(device_, &pipelineLayout, nullptr, &pipelineLayout_));
        shader(shaders / "copy-vertex.spv");
        shader(shaders / "copy-fragment.spv");
        std::cout << "validation_layer=" << validationEnabled_ << " surface=none\n";
    }

    void run(const std::vector<std::array<float, 4>>& fixture) {
        auto& staging = buffer(fixture.size() * sizeof(fixture[0]), VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        void* mapped = nullptr;
        check(load<PFN_vkMapMemory>(library_, "vkMapMemory")(device_, staging.memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        std::memcpy(mapped, fixture.data(), fixture.size() * sizeof(fixture[0]));
        load<PFN_vkUnmapMemory>(library_, "vkUnmapMemory")(device_, staging.memory);
        auto& input = image(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        begin();
        transition(input, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        VkBufferImageCopy copy{};
        copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copy.imageExtent = {width_, 1, 1};
        load<PFN_vkCmdCopyBufferToImage>(library_, "vkCmdCopyBufferToImage")(
            command_, staging.handle, input.handle, input.layout, 1, &copy);
        transition(input, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        submit();
        const auto inputSet = descriptor(input.views[0]);
        auto& readback = buffer(width_ * 4, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        size_t encodingChecks = 0, copyChecks = 0;
        unsigned maximumError = 0;
        for (bool bgra : {false, true}) {
            const VkFormat linear = bgra ? VK_FORMAT_B8G8R8A8_UNORM : VK_FORMAT_R8G8B8A8_UNORM;
            const VkFormat srgb = bgra ? VK_FORMAT_B8G8R8A8_SRGB : VK_FORMAT_R8G8B8A8_SRGB;
            const VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            auto& encoded = image(linear, usage, srgb);
            auto& displayed = image(linear, usage, srgb);
            const auto encodedSet = descriptor(encoded.views[0]);
            const auto wrongReadSet = descriptor(encoded.views[1]);
            const auto encodePipeline = pipeline(srgb), copyPipeline = pipeline(linear);
            const auto firstTarget = framebuffer(encodePipeline.first, encoded.views[1]);
            const auto copyTarget = framebuffer(copyPipeline.first, displayed.views[0]);
            const auto wrongWriteTarget = framebuffer(encodePipeline.first, displayed.views[1]);
            const auto first = drawCopy(input, inputSet, encoded, firstTarget, encodePipeline, readback, bgra);
            for (uint32_t index = 0; index < width_; ++index) {
                for (size_t channel = 0; channel < 3; ++channel) {
                    const int expected = viewportCopySrgbByteCandidate(fixture[index][channel]);
                    const unsigned error = static_cast<unsigned>(std::abs(int(first[index][channel]) - expected));
                    maximumError = std::max(maximumError, error);
                    if (error > 1) throw std::runtime_error("Vulkan encoding exceeds one-byte tolerance");
                    ++encodingChecks;
                }
            }
            const auto second = drawCopy(encoded, encodedSet, displayed, copyTarget, copyPipeline, readback, bgra);
            if (first != second) throw std::runtime_error("Vulkan second copy changed RGB bytes");
            copyChecks += width_ * 3;
            reportViewportCopyPixelsCandidate("vulkan", static_cast<unsigned>(linear), first, second);
            const auto decoded = drawCopy(encoded, wrongReadSet, displayed, copyTarget, copyPipeline, readback, bgra);
            const auto reencoded = drawCopy(encoded, encodedSet, displayed, wrongWriteTarget, encodePipeline, readback, bgra);
            size_t decodeDifferences = 0, encodeDifferences = 0;
            for (uint32_t index = 0; index < width_; ++index) {
                if (decoded[index] != first[index]) ++decodeDifferences;
                if (reencoded[index] != first[index]) ++encodeDifferences;
            }
            if (decodeDifferences < 200 || encodeDifferences < 200)
                throw std::runtime_error("Vulkan negative controls failed");
            std::cout << "target_format=" << linear << " pixels=" << width_ << " copy_rgb_exact=1"
                      << " wrong_read_changed_pixels=" << decodeDifferences
                      << " wrong_write_changed_pixels=" << encodeDifferences << '\n';
        }
        if (validationErrors_) throw std::runtime_error("Vulkan validation error");
        std::cout << "PASS native_x64=1 encoding_checks=" << encodingChecks << " copy_checks=" << copyChecks
                  << " max_encoding_byte_error=" << maximumError << " validation_errors=" << validationErrors_
                  << " original_sfm_execution=0\n";
    }
};

int main(int argc, char** argv) {
    static_assert(sizeof(void*) == 8, "native x64 fixture required");
    try {
        if (argc != 2) throw std::runtime_error("usage: viewport_copy_vulkan_x64 SHADER_DIRECTORY");
        const auto fixture = viewportCopyFixtureCandidate();
        CopyFixture context;
        context.initialize(static_cast<uint32_t>(fixture.size()), argv[1]);
        context.run(fixture);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
