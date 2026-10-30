#define NOMINMAX
#define VK_NO_PROTOTYPES
#include <windows.h>
#include <vulkan/vulkan.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>
#include "model_render_inputs_candidate.h"
template<class T> T load(HMODULE library, const char* name) {
    auto address = GetProcAddress(library, name);
    if (!address) throw std::runtime_error(name);
    return reinterpret_cast<T>(address);
}
#include "vulkan_material_images_candidate.h"
#include "vulkan_offscreen_target_candidate.h"
#include "vulkan_prepared_scene_candidate.h"
#include "vulkan_scene_vertex_input_candidate.h"

static void need(bool value, const char* reason) { if (!value) throw std::runtime_error(reason); }
static void check(VkResult value) { if (value != VK_SUCCESS) throw std::runtime_error("Vulkan result " + std::to_string(value)); }
static std::vector<uint32_t> words(const char* path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    need(bool(file), "shader input missing");
    auto length = file.tellg();
    need(length >= 20 && uint64_t(length) % 4 == 0, "shader input size");
    std::vector<uint32_t> result(static_cast<size_t>(length) / 4);
    file.seekg(0); file.read(reinterpret_cast<char*>(result.data()), length);
    need(bool(file) && result[0] == 0x07230203 && result[1] == 0x00010000, "SPIR-V 1.0 input");
    return result;
}
static void vertexInterface(const std::vector<uint32_t>& shader) {
    std::map<uint32_t, uint32_t> locations, storage;
    for (size_t i = 5; i < shader.size();) {
        const uint32_t count = shader[i] >> 16, op = shader[i] & 65535;
        need(count && count <= shader.size() - i, "SPIR-V instruction bounds");
        if (op == 71 && count == 4 && shader[i+2] == 30) locations[shader[i+1]] = shader[i+3];
        if (op == 59 && count >= 4) storage[shader[i+2]] = shader[i+3];
        i += count;
    }
    std::set<uint32_t> inputs, outputs;
    for (const auto& entry : locations) {
        if (storage[entry.first] == 1) inputs.insert(entry.second);
        if (storage[entry.first] == 3) outputs.insert(entry.second);
    }
    need(inputs == std::set<uint32_t>({0,1,2,3}) && outputs == std::set<uint32_t>({0,1,2}),
         "vertex shader lacks displacement alpha interface");
}

struct Context {
    HMODULE library = nullptr;
    VkInstance instance{}; VkPhysicalDevice physical{}; VkDevice device{}; VkQueue queue{};
    uint32_t family = 0;
    std::vector<std::function<void()>> cleanup;
    ~Context() {
        if (device) {
            load<PFN_vkDeviceWaitIdle>(library,"vkDeviceWaitIdle")(device);
            for (auto i=cleanup.rbegin();i!=cleanup.rend();++i) (*i)();
            load<PFN_vkDestroyDevice>(library,"vkDestroyDevice")(device,nullptr);
        }
        if (instance) load<PFN_vkDestroyInstance>(library,"vkDestroyInstance")(instance,nullptr);
        if (library) FreeLibrary(library);
    }
};
#define FN(name) load<PFN_##name>(c.library, #name)
static void initialize(Context& c) {
    c.library = LoadLibraryExA("vulkan-1.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    need(c.library != nullptr, "system Vulkan loader missing");
    VkInstanceCreateInfo ii{}; ii.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    check(FN(vkCreateInstance)(&ii,nullptr,&c.instance));
    uint32_t count=0; check(FN(vkEnumeratePhysicalDevices)(c.instance,&count,nullptr));
    std::vector<VkPhysicalDevice> devices(count); check(FN(vkEnumeratePhysicalDevices)(c.instance,&count,devices.data()));
    for (auto device : devices) {
        VkPhysicalDeviceProperties properties{}; FN(vkGetPhysicalDeviceProperties)(device,&properties);
        if (properties.limits.maxPushConstantsSize < 144) continue;
        uint32_t n=0; FN(vkGetPhysicalDeviceQueueFamilyProperties)(device,&n,nullptr);
        std::vector<VkQueueFamilyProperties> families(n); FN(vkGetPhysicalDeviceQueueFamilyProperties)(device,&n,families.data());
        for (uint32_t f=0;f<n;++f) if (families[f].queueCount && (families[f].queueFlags&VK_QUEUE_GRAPHICS_BIT)) {
            c.physical=device; c.family=f; break;
        }
        if (c.physical) break;
    }
    need(c.physical != VK_NULL_HANDLE, "graphics device with 144-byte push constants unavailable");
    float priority=1; VkDeviceQueueCreateInfo qi{}; qi.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qi.queueFamilyIndex=c.family; qi.queueCount=1; qi.pQueuePriorities=&priority;
    VkDeviceCreateInfo di{}; di.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO; di.queueCreateInfoCount=1; di.pQueueCreateInfos=&qi;
    check(FN(vkCreateDevice)(c.physical,&di,nullptr,&c.device)); FN(vkGetDeviceQueue)(c.device,c.family,0,&c.queue);
}
struct Buffer { VkBuffer buffer{}; VkDeviceMemory memory{}; VkDeviceSize size=0; };
static Buffer buffer(Context& c, VkDeviceSize size, VkBufferUsageFlags usage) {
    Buffer b; b.size=size;
    VkBufferCreateInfo bi{}; bi.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO; bi.size=size; bi.usage=usage;
    check(FN(vkCreateBuffer)(c.device,&bi,nullptr,&b.buffer));
    c.cleanup.push_back([&c,h=b.buffer]{ FN(vkDestroyBuffer)(c.device,h,nullptr); });
    VkMemoryRequirements requirements{}; FN(vkGetBufferMemoryRequirements)(c.device,b.buffer,&requirements);
    VkPhysicalDeviceMemoryProperties properties{}; FN(vkGetPhysicalDeviceMemoryProperties)(c.physical,&properties);
    uint32_t type=UINT32_MAX; const auto flags=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    for (uint32_t i=0;i<properties.memoryTypeCount;++i)
        if ((requirements.memoryTypeBits&(1u<<i)) && (properties.memoryTypes[i].propertyFlags&flags)==flags) {type=i;break;}
    need(type!=UINT32_MAX,"coherent test buffer memory unavailable");
    VkMemoryAllocateInfo ai{}; ai.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO; ai.allocationSize=requirements.size; ai.memoryTypeIndex=type;
    check(FN(vkAllocateMemory)(c.device,&ai,nullptr,&b.memory));
    // Reverse cleanup destroys the buffer before freeing its bound memory.
    auto destroy=c.cleanup.back(); c.cleanup.pop_back();
    c.cleanup.push_back([&c,m=b.memory]{FN(vkFreeMemory)(c.device,m,nullptr);}); c.cleanup.push_back(destroy);
    check(FN(vkBindBufferMemory)(c.device,b.buffer,b.memory,0)); return b;
}
static void upload(Context& c, const Buffer& buffer, const void* data, size_t size) {
    need(size<=buffer.size,"test buffer upload size"); void* mapped=nullptr;
    check(FN(vkMapMemory)(c.device,buffer.memory,0,size,0,&mapped)); std::memcpy(mapped,data,size); FN(vkUnmapMemory)(c.device,buffer.memory);
}

int main(int argc,char** argv) {
    try {
        static_assert(sizeof(void*)==8,"x64 required");
        need(argc==4,"usage: probe vertex.spv fragment.spv fresh-output-directory");
        auto vertexWords=words(argv[1]), fragmentWords=words(argv[2]); vertexInterface(vertexWords);
        const std::filesystem::path output(argv[3]); need(std::filesystem::is_directory(output),"output directory required");
        Context c; initialize(c);
        auto attributes=vulkanSceneVertexAttributesCandidate(); auto binding=vulkanSceneVertexBindingCandidate();
        VkDescriptorSetLayoutBinding bindings[4]{{0,VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
            {1,VK_DESCRIPTOR_TYPE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
            {2,VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
            {3,VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}};
        VkDescriptorSetLayoutCreateInfo si{}; si.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO; si.bindingCount=4; si.pBindings=bindings;
        VkDescriptorSetLayout set{}; check(FN(vkCreateDescriptorSetLayout)(c.device,&si,nullptr,&set));
        c.cleanup.push_back([&c,set]{FN(vkDestroyDescriptorSetLayout)(c.device,set,nullptr);});
        VkPushConstantRange ranges[2]{{VK_SHADER_STAGE_VERTEX_BIT,0,64},{VK_SHADER_STAGE_FRAGMENT_BIT,96,48}};
        VkPipelineLayoutCreateInfo li{}; li.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO; li.setLayoutCount=1; li.pSetLayouts=&set; li.pushConstantRangeCount=2; li.pPushConstantRanges=ranges;
        VkPipelineLayout layout{}; check(FN(vkCreatePipelineLayout)(c.device,&li,nullptr,&layout));
        c.cleanup.push_back([&c,layout]{FN(vkDestroyPipelineLayout)(c.device,layout,nullptr);});
        VkAttachmentDescription attachments[2]{};
        attachments[0].format=VK_FORMAT_R8G8B8A8_UNORM; attachments[0].samples=VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR; attachments[0].storeOp=VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE; attachments[0].stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout=VK_IMAGE_LAYOUT_UNDEFINED; attachments[0].finalLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments[1]=attachments[0]; attachments[1].format=VK_FORMAT_D32_SFLOAT; attachments[1].finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        VkAttachmentReference color{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}, depth{1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        VkSubpassDescription sub{}; sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS; sub.colorAttachmentCount=1; sub.pColorAttachments=&color; sub.pDepthStencilAttachment=&depth;
        VkSubpassDependency dependency{}; dependency.srcSubpass=VK_SUBPASS_EXTERNAL; dependency.dstSubpass=0;
        dependency.srcStageMask=VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; dependency.dstStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        VkRenderPassCreateInfo ri{}; ri.sType=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO; ri.attachmentCount=2; ri.pAttachments=attachments; ri.subpassCount=1; ri.pSubpasses=&sub; ri.dependencyCount=1; ri.pDependencies=&dependency;
        VkRenderPass pass{}; check(FN(vkCreateRenderPass)(c.device,&ri,nullptr,&pass));
        c.cleanup.push_back([&c,pass]{FN(vkDestroyRenderPass)(c.device,pass,nullptr);});
        auto shader=[&](const std::vector<uint32_t>& words){ VkShaderModuleCreateInfo mi{}; mi.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO; mi.codeSize=words.size()*4; mi.pCode=words.data(); VkShaderModule module{}; check(FN(vkCreateShaderModule)(c.device,&mi,nullptr,&module)); c.cleanup.push_back([&c,module]{FN(vkDestroyShaderModule)(c.device,module,nullptr);}); return module; };
        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType=stages[1].sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT; stages[0].module=shader(vertexWords); stages[0].pName="main";
        stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT; stages[1].module=shader(fragmentWords); stages[1].pName="main";
        VkPipelineVertexInputStateCreateInfo vi{}; vi.sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO; vi.vertexBindingDescriptionCount=1; vi.pVertexBindingDescriptions=&binding; vi.vertexAttributeDescriptionCount=uint32_t(attributes.size()); vi.pVertexAttributeDescriptions=attributes.data();
        VkPipelineInputAssemblyStateCreateInfo ia{}; ia.sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO; ia.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        constexpr uint32_t width=64,height=32;
        VkViewport viewport{0,0,float(width),float(height),0,1}; VkRect2D scissor{{0,0},{width,height}};
        VkPipelineViewportStateCreateInfo vp{}; vp.sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO; vp.viewportCount=vp.scissorCount=1; vp.pViewports=&viewport; vp.pScissors=&scissor;
        VkPipelineRasterizationStateCreateInfo raster{}; raster.sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO; raster.polygonMode=VK_POLYGON_MODE_FILL; raster.cullMode=VK_CULL_MODE_NONE; raster.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE; raster.lineWidth=1;
        VkPipelineMultisampleStateCreateInfo ms{}; ms.sType=VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO; ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo ds{}; ds.sType=VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        VkPipelineColorBlendAttachmentState ca{}; ca.colorWriteMask=15;
        VkPipelineColorBlendStateCreateInfo cb{}; cb.sType=VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO; cb.attachmentCount=1; cb.pAttachments=&ca;
        VkGraphicsPipelineCreateInfo pi{}; pi.sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO; pi.stageCount=2; pi.pStages=stages; pi.pVertexInputState=&vi; pi.pInputAssemblyState=&ia; pi.pViewportState=&vp; pi.pRasterizationState=&raster; pi.pMultisampleState=&ms; pi.pDepthStencilState=&ds; pi.pColorBlendState=&cb; pi.layout=layout; pi.renderPass=pass;
        VkPipeline pipeline{}; check(FN(vkCreateGraphicsPipelines)(c.device,VK_NULL_HANDLE,1,&pi,nullptr,&pipeline));
        c.cleanup.push_back([&c,pipeline]{FN(vkDestroyPipeline)(c.device,pipeline,nullptr);});
        VulkanOffscreenTargetCandidate target(c.library,c.device,c.physical,pass,width,height);
        ModelRenderMaterialCandidate material; material.image={1,1,{32,96,160,255}}; material.secondBaseImage={1,1,{224,160,64,255}};
        VulkanMaterialImagesCandidate images(c.library,c.device,c.physical); images.create({material},set);
        SceneGpuUploadCandidate scene; scene.vertices={{{-1,-1,.5f},{1,1,1},{.5f,.5f}},{{1,-1,.5f},{1,1,1},{.5f,.5f}},{{-1,1,.5f},{1,1,1},{.5f,.5f}},{{1,1,.5f},{1,1,1},{.5f,.5f}}};
        scene.indices={0,1,2,2,1,3}; scene.materials={{0,0}}; scene.opaque={{0,0,0,6,0,0}};
        auto vertices=buffer(c,scene.vertices.size()*sizeof(SceneGpuVertexCandidate),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        auto indices=buffer(c,scene.indices.size()*4,VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        auto readback=buffer(c,width*height*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT); upload(c,indices,scene.indices.data(),scene.indices.size()*4);
        VkCommandPoolCreateInfo ci{}; ci.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO; ci.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; ci.queueFamilyIndex=c.family;
        VkCommandPool pool{}; check(FN(vkCreateCommandPool)(c.device,&ci,nullptr,&pool)); c.cleanup.push_back([&c,pool]{FN(vkDestroyCommandPool)(c.device,pool,nullptr);});
        VkCommandBufferAllocateInfo ai{}; ai.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO; ai.commandPool=pool; ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY; ai.commandBufferCount=1;
        VkCommandBuffer command{}; check(FN(vkAllocateCommandBuffers)(c.device,&ai,&command));
        struct IdleGuard {Context& c; ~IdleGuard(){FN(vkDeviceWaitIdle)(c.device);} } idle{c};
        VulkanSceneCommandFunctionsCandidate fn{FN(vkCmdBindPipeline),FN(vkCmdBindDescriptorSets),FN(vkCmdBindVertexBuffers),FN(vkCmdBindIndexBuffer),FN(vkCmdDrawIndexed)};
        VulkanPreparedSceneResourcesCandidate resources{}; resources.vertices=vertices.buffer; resources.indices=indices.buffer; resources.vertexCapacity=vertices.size; resources.indexCapacity=indices.size; resources.opaque=resources.translucent=pipeline; resources.layout=layout; resources.materials={images.entries[0].set}; resources.vertexTransition={1}; resources.vertexLighting={0};
        RenderCameraCandidate camera{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
        struct Case {const char* name;float left,right;bool transition,lighting;float alpha;};
        const Case cases[]={{"disabled",255,255,false,false,1},{"negative",-127,-127,true,false,1},
            {"zero",0,0,true,false,1},{"quarter",63.75f,63.75f,true,false,1},{"half",127.5f,127.5f,true,false,1},
            {"full",255,255,true,false,1},{"above",510,510,true,false,1},{"gradient",0,255,true,false,1},
            {"reverse",255,0,true,false,1},{"clamped-gradient",-255,510,true,false,1},
            {"lit-half",127.5f,127.5f,true,true,1},{"draw-alpha",127.5f,127.5f,true,false,.5f}};
        unsigned sample=0;
        for (const auto& test : cases) {
            for (auto& vertex : scene.vertices) {vertex.displacementAlpha=vertex.position[0]<0?test.left:test.right;vertex.normal={.25f,.5f,.75f};need(sceneGpuVertexFiniteCandidate(vertex),"finite controlled vertex");}
            upload(c,vertices,scene.vertices.data(),scene.vertices.size()*sizeof(SceneGpuVertexCandidate));
            resources.vertexTransition[0]=test.transition?1.f:0.f; resources.vertexLighting[0]=test.lighting?1.f:0.f;
            check(FN(vkResetCommandBuffer)(command,0)); VkCommandBufferBeginInfo begin{}; begin.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO; check(FN(vkBeginCommandBuffer)(command,&begin));
            if (sample==0) images.upload(command);
            VkClearValue clears[2]{}; clears[1].depthStencil.depth=1;
            VkRenderPassBeginInfo render{}; render.sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO; render.renderPass=pass; render.framebuffer=target.framebuffer; render.renderArea=scissor; render.clearValueCount=2; render.pClearValues=clears;
            FN(vkCmdBeginRenderPass)(command,&render,VK_SUBPASS_CONTENTS_INLINE);
            recordVulkanPreparedSceneCandidate(fn,FN(vkCmdPushConstants),command,scene,camera,{test.alpha},resources);
            FN(vkCmdEndRenderPass)(command);
            VkImageMemoryBarrier barrier{}; barrier.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER; barrier.oldLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL; barrier.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; barrier.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT; barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED; barrier.image=target.color; barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
            FN(vkCmdPipelineBarrier)(command,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
            VkBufferImageCopy copy{}; copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1}; copy.imageExtent={width,height,1}; FN(vkCmdCopyImageToBuffer)(command,target.color,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,readback.buffer,1,&copy);
            VkMemoryBarrier host{}; host.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER; host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT; host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
            FN(vkCmdPipelineBarrier)(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&host,0,nullptr,0,nullptr);
            check(FN(vkEndCommandBuffer)(command)); VkSubmitInfo submit{}; submit.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO; submit.commandBufferCount=1; submit.pCommandBuffers=&command;
            check(FN(vkQueueSubmit)(c.queue,1,&submit,VK_NULL_HANDLE)); check(FN(vkQueueWaitIdle)(c.queue));
            std::vector<uint8_t> pixels(width*height*4);void* mapped=nullptr;check(FN(vkMapMemory)(c.device,readback.memory,0,pixels.size(),0,&mapped));std::memcpy(pixels.data(),mapped,pixels.size());FN(vkUnmapMemory)(c.device,readback.memory);
            const auto path=output/(std::string(test.name)+".rgba");need(!std::filesystem::exists(path),"fresh result file required");std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<const char*>(pixels.data()),std::streamsize(pixels.size()));need(bool(file),"result write failed");++sample;
        }
        std::cout<<"PASS production_vertex_transition_draws="<<sample<<" width="<<width<<" height="<<height<<" shared_stride="<<binding.stride<<" pixel_comparison=external native_source_parity=unverified\n";
        return 0;
    } catch (const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
