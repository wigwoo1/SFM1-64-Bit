#define NOMINMAX
#define VK_NO_PROTOTYPES
#include <windows.h>
#include <vulkan/vulkan.h>
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "vulkan_prepared_scene_candidate.h"

struct Context {
    HMODULE library=nullptr; VkInstance instance{}; VkDevice device{};
    VkPhysicalDevice physical{}; VkQueue queue{}; uint32_t family=0;
    VkPhysicalDeviceMemoryProperties memory{};
    std::vector<std::function<void()>> cleanup;
    template<class T>T load(const char* name){auto p=GetProcAddress(library,name);if(!p)throw std::runtime_error(name);return reinterpret_cast<T>(p);}
    static void check(VkResult result){if(result!=VK_SUCCESS)throw std::runtime_error("Vulkan result="+std::to_string(result));}
    ~Context(){
        if(device){load<PFN_vkDeviceWaitIdle>("vkDeviceWaitIdle")(device);for(auto it=cleanup.rbegin();it!=cleanup.rend();++it)(*it)();load<PFN_vkDestroyDevice>("vkDestroyDevice")(device,nullptr);}
        if(instance)load<PFN_vkDestroyInstance>("vkDestroyInstance")(instance,nullptr);
        if(library)FreeLibrary(library);
    }
    uint32_t memoryType(uint32_t bits,VkMemoryPropertyFlags flags){
        for(uint32_t i=0;i<memory.memoryTypeCount;++i)if((bits&(1u<<i))&&(memory.memoryTypes[i].propertyFlags&flags)==flags)return i;
        throw std::runtime_error("required memory type unavailable");
    }
};
#define CALL(name) c.load<PFN_##name>(#name)
#define OK(expression) Context::check(expression)
struct Image {VkImage image{};VkImageView view{};VkDeviceMemory memory{};};
Image makeImage(Context&c,uint32_t width,uint32_t height,VkFormat format,VkImageUsageFlags usage,VkImageAspectFlags aspect,bool mutableFormat=false){
    Image result;VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    info.flags=mutableFormat?VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT:0;info.imageType=VK_IMAGE_TYPE_2D;
    info.format=format;info.extent={width,height,1};info.mipLevels=info.arrayLayers=1;
    info.samples=VK_SAMPLE_COUNT_1_BIT;info.tiling=VK_IMAGE_TILING_OPTIMAL;info.usage=usage;
    OK(CALL(vkCreateImage)(c.device,&info,nullptr,&result.image));
    VkMemoryRequirements requirements{};CALL(vkGetImageMemoryRequirements)(c.device,result.image,&requirements);
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=requirements.size;
    allocation.memoryTypeIndex=c.memoryType(requirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    OK(CALL(vkAllocateMemory)(c.device,&allocation,nullptr,&result.memory));
    c.cleanup.push_back([&,value=result.memory]{CALL(vkFreeMemory)(c.device,value,nullptr);});
    c.cleanup.push_back([&,value=result.image]{CALL(vkDestroyImage)(c.device,value,nullptr);});
    OK(CALL(vkBindImageMemory)(c.device,result.image,result.memory,0));
    VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};view.image=result.image;view.viewType=VK_IMAGE_VIEW_TYPE_2D;
    view.format=format;view.subresourceRange={aspect,0,1,0,1};
    OK(CALL(vkCreateImageView)(c.device,&view,nullptr,&result.view));
    c.cleanup.push_back([&,value=result.view]{CALL(vkDestroyImageView)(c.device,value,nullptr);});return result;
}
struct Buffer {VkBuffer buffer{};VkDeviceMemory memory{};};
Buffer makeBuffer(Context&c,VkDeviceSize size,VkBufferUsageFlags usage){
    Buffer result;VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};info.size=size;info.usage=usage;
    OK(CALL(vkCreateBuffer)(c.device,&info,nullptr,&result.buffer));
    VkMemoryRequirements requirements{};CALL(vkGetBufferMemoryRequirements)(c.device,result.buffer,&requirements);
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=requirements.size;
    allocation.memoryTypeIndex=c.memoryType(requirements.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    OK(CALL(vkAllocateMemory)(c.device,&allocation,nullptr,&result.memory));
    c.cleanup.push_back([&,value=result.memory]{CALL(vkFreeMemory)(c.device,value,nullptr);});
    c.cleanup.push_back([&,value=result.buffer]{CALL(vkDestroyBuffer)(c.device,value,nullptr);});
    OK(CALL(vkBindBufferMemory)(c.device,result.buffer,result.memory,0));return result;
}
int main(int argc,char**argv){
    static_assert(sizeof(void*)==8,"x64 required");
    try{
        const bool production=argc==4&&std::string(argv[3])=="--production-transport";if(argc!=3&&!production)throw std::runtime_error("usage: probe shader-output-directory fresh-capture-directory [--production-transport]");
        const std::filesystem::path shaders=argv[1],output=argv[2];
        if(!std::filesystem::create_directory(output))throw std::runtime_error("fresh output directory required");
        Context c;c.library=LoadLibraryExW(L"vulkan-1.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
        if(!c.library)throw std::runtime_error("Vulkan loader missing");
        VkInstanceCreateInfo instanceInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        OK(CALL(vkCreateInstance)(&instanceInfo,nullptr,&c.instance));
        uint32_t count=0;OK(CALL(vkEnumeratePhysicalDevices)(c.instance,&count,nullptr));
        std::vector<VkPhysicalDevice> devices(count);OK(CALL(vkEnumeratePhysicalDevices)(c.instance,&count,devices.data()));
        for(auto physical:devices){
            uint32_t families=0;CALL(vkGetPhysicalDeviceQueueFamilyProperties)(physical,&families,nullptr);
            std::vector<VkQueueFamilyProperties> properties(families);CALL(vkGetPhysicalDeviceQueueFamilyProperties)(physical,&families,properties.data());
            for(uint32_t i=0;i<families;++i)if(properties[i].queueCount&&(properties[i].queueFlags&VK_QUEUE_GRAPHICS_BIT)){c.physical=physical;c.family=i;break;}
            if(c.physical)break;
        }
        if(!c.physical)throw std::runtime_error("graphics device missing");
        VkPhysicalDeviceProperties properties{};CALL(vkGetPhysicalDeviceProperties)(c.physical,&properties);
        std::cout<<"adapter="<<properties.deviceName<<" x64=1\n";
        if(production&&properties.limits.maxPushConstantsSize<176)throw std::runtime_error("production material needs 176 push-constant bytes");
        float priority=1;VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queueInfo.queueFamilyIndex=c.family;queueInfo.queueCount=1;queueInfo.pQueuePriorities=&priority;
        VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};deviceInfo.queueCreateInfoCount=1;deviceInfo.pQueueCreateInfos=&queueInfo;
        OK(CALL(vkCreateDevice)(c.physical,&deviceInfo,nullptr,&c.device));
        CALL(vkGetDeviceQueue)(c.device,c.family,0,&c.queue);CALL(vkGetPhysicalDeviceMemoryProperties)(c.physical,&c.memory);
        VkCommandPool pool{};VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        poolInfo.queueFamilyIndex=c.family;poolInfo.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        OK(CALL(vkCreateCommandPool)(c.device,&poolInfo,nullptr,&pool));
        c.cleanup.push_back([&c,pool]{CALL(vkDestroyCommandPool)(c.device,pool,nullptr);});
        VkCommandBuffer command{};VkCommandBufferAllocateInfo commandInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        commandInfo.commandPool=pool;commandInfo.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;commandInfo.commandBufferCount=1;
        OK(CALL(vkAllocateCommandBuffers)(c.device,&commandInfo,&command));
        auto begin=[&]{OK(CALL(vkResetCommandBuffer)(command,0));VkCommandBufferBeginInfo b{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};b.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;OK(CALL(vkBeginCommandBuffer)(command,&b));};
        auto submit=[&]{OK(CALL(vkEndCommandBuffer)(command));VkSubmitInfo s{VK_STRUCTURE_TYPE_SUBMIT_INFO};s.commandBufferCount=1;s.pCommandBuffers=&command;OK(CALL(vkQueueSubmit)(c.queue,1,&s,VK_NULL_HANDLE));OK(CALL(vkQueueWaitIdle)(c.queue));};
        auto staging=makeBuffer(c,32,VK_BUFFER_USAGE_TRANSFER_SRC_BIT);auto readback=makeBuffer(c,64*64*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        const uint8_t textures[]{128,128,64,0,64,128,128,255,32,64,96,255,96,128,32,255,
                                  128,64,64,0,64,128,128,255,128,96,32,255,32,96,128,255};
        void* mapped=nullptr;OK(CALL(vkMapMemory)(c.device,staging.memory,0,32,0,&mapped));std::memcpy(mapped,textures,32);CALL(vkUnmapMemory)(c.device,staging.memory);
        std::array<Image,2> textureImages;
        std::array<VkImageView,2> srgbViews{};
        for(size_t i=0;i<2;++i){
            textureImages[i]=makeImage(c,2,2,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,VK_IMAGE_ASPECT_COLOR_BIT,true);
            VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};view.image=textureImages[i].image;view.viewType=VK_IMAGE_VIEW_TYPE_2D;
            view.format=VK_FORMAT_R8G8B8A8_SRGB;view.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
            OK(CALL(vkCreateImageView)(c.device,&view,nullptr,&srgbViews[i]));
            c.cleanup.push_back([&c,value=srgbViews[i]]{CALL(vkDestroyImageView)(c.device,value,nullptr);});
        }
        begin();
        for(size_t i=0;i<2;++i){
            VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
            barrier.image=textureImages[i].image;barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
            barrier.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
            CALL(vkCmdPipelineBarrier)(command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
            VkBufferImageCopy copy{};copy.bufferOffset=i*16;copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={2,2,1};
            CALL(vkCmdCopyBufferToImage)(command,staging.buffer,textureImages[i].image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
            barrier.oldLayout=barrier.newLayout;barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
            CALL(vkCmdPipelineBarrier)(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
        }submit();
        VkSampler sampler{};VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        samplerInfo.magFilter=samplerInfo.minFilter=VK_FILTER_NEAREST;samplerInfo.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.addressModeU=samplerInfo.addressModeV=samplerInfo.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        OK(CALL(vkCreateSampler)(c.device,&samplerInfo,nullptr,&sampler));c.cleanup.push_back([&c,sampler]{CALL(vkDestroySampler)(c.device,sampler,nullptr);});
        VkDescriptorSetLayout setLayout{};VkDescriptorSetLayoutBinding bindings[]{
            {0,VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
            {1,VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
            {2,VK_DESCRIPTOR_TYPE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}};
        if(production){bindings[1].binding=3;bindings[2].binding=1;}
        VkDescriptorSetLayoutCreateInfo setInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};setInfo.bindingCount=3;setInfo.pBindings=bindings;
        OK(CALL(vkCreateDescriptorSetLayout)(c.device,&setInfo,nullptr,&setLayout));c.cleanup.push_back([&c,setLayout]{CALL(vkDestroyDescriptorSetLayout)(c.device,setLayout,nullptr);});
        VkPipelineLayout layout{};VkPushConstantRange push[]{{VK_SHADER_STAGE_VERTEX_BIT,0,80},{VK_SHADER_STAGE_FRAGMENT_BIT,96,80}};
        VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};layoutInfo.setLayoutCount=1;layoutInfo.pSetLayouts=&setLayout;
        layoutInfo.pushConstantRangeCount=production?2u:1u;layoutInfo.pPushConstantRanges=push;
        OK(CALL(vkCreatePipelineLayout)(c.device,&layoutInfo,nullptr,&layout));c.cleanup.push_back([&c,layout]{CALL(vkDestroyPipelineLayout)(c.device,layout,nullptr);});
        VkDescriptorPool descriptorPool{};VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,2},{VK_DESCRIPTOR_TYPE_SAMPLER,1}};
        VkDescriptorPoolCreateInfo descriptorInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};descriptorInfo.maxSets=1;descriptorInfo.poolSizeCount=2;descriptorInfo.pPoolSizes=sizes;
        OK(CALL(vkCreateDescriptorPool)(c.device,&descriptorInfo,nullptr,&descriptorPool));c.cleanup.push_back([&c,descriptorPool]{CALL(vkDestroyDescriptorPool)(c.device,descriptorPool,nullptr);});
        VkDescriptorSet set{};VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};allocate.descriptorPool=descriptorPool;allocate.descriptorSetCount=1;allocate.pSetLayouts=&setLayout;
        OK(CALL(vkAllocateDescriptorSets)(c.device,&allocate,&set));
        auto shader=[&](const char*name){
            std::ifstream file(shaders/name,std::ios::binary|std::ios::ate);if(!file)throw std::runtime_error("SPIR-V missing");
            auto size=file.tellg();if(size<20||uint64_t(size)%4)throw std::runtime_error("invalid SPIR-V size");
            std::vector<uint32_t> words(size_t(size)/4);file.seekg(0);file.read(reinterpret_cast<char*>(words.data()),size);if(!file)throw std::runtime_error("shader read failed");
            VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};info.codeSize=size_t(size);info.pCode=words.data();VkShaderModule result{};
            OK(CALL(vkCreateShaderModule)(c.device,&info,nullptr,&result));c.cleanup.push_back([&c,result]{CALL(vkDestroyShaderModule)(c.device,result,nullptr);});return result;
        };
        auto vs=shader(production?"two-texture-raw-vs.spv":"two-texture-reference-vs.spv"),ps=shader(production?"sfm64-unlit-two-texture-candidate.spv":"two-texture-reference-ps.spv");
        auto vertexBuffer=makeBuffer(c,3*sizeof(SceneGpuVertexCandidate),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);auto indexBuffer=makeBuffer(c,12,VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        const uint32_t indices[]{0,1,2};OK(CALL(vkMapMemory)(c.device,indexBuffer.memory,0,12,0,&mapped));std::memcpy(mapped,indices,12);CALL(vkUnmapMemory)(c.device,indexBuffer.memory);
        struct Case{const char*name;bool srgb,additive,alphaWrite,depthWrite;float shift0,shift1,clearDepth;int second;bool unormOutput=false;};
        const Case cases[]{
            {"linear-product",false,false,true,false,0,0,1,0}, {"second-uv-only",false,false,true,false,0,.5f,1,0},
            {"first-uv-only",false,false,true,false,.5f,0,1,0}, {"additive",false,true,true,false,0,0,1,0},
            {"alpha-write-off",false,true,false,false,0,0,1,0}, {"occluded",false,true,true,false,0,0,.3f,0},
            {"depth-write-off",false,false,true,false,0,0,1,1}, {"depth-write-on-control",false,false,true,true,0,0,1,1},
            {"srgb-product",true,false,true,false,0,0,1,0},
            {"srgb-clear-occluded",true,false,true,false,0,0,.3f,0}, {"srgb-additive",true,true,true,false,0,0,1,0},
            {"srgb-layered",true,true,true,false,0,0,1,1},
            {"srgb-layered-unorm-control",true,true,true,false,0,0,1,1,true},
            {"drawn-occluder",false,false,true,true,0,0,1,2}};
        std::ofstream receipt(output/"pixels.csv");receipt<<"case,r,g,b,a\n";
        for(const auto& test:cases){
            const auto format=test.srgb&&!test.unormOutput?VK_FORMAT_R8G8B8A8_SRGB:VK_FORMAT_R8G8B8A8_UNORM;
            auto color=makeImage(c,64,64,format,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT,VK_IMAGE_ASPECT_COLOR_BIT);
            auto depth=makeImage(c,64,64,VK_FORMAT_D32_SFLOAT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,VK_IMAGE_ASPECT_DEPTH_BIT);
            VkAttachmentDescription attachments[2]{};
            attachments[0].format=format;attachments[0].samples=VK_SAMPLE_COUNT_1_BIT;attachments[0].loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;attachments[0].storeOp=VK_ATTACHMENT_STORE_OP_STORE;
            attachments[0].initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;attachments[0].finalLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            attachments[1]=attachments[0];attachments[1].format=VK_FORMAT_D32_SFLOAT;attachments[1].storeOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;attachments[1].finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            VkAttachmentReference colorRef{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},depthRef{1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
            VkSubpassDescription subpass{};subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;subpass.colorAttachmentCount=1;subpass.pColorAttachments=&colorRef;subpass.pDepthStencilAttachment=&depthRef;
            VkSubpassDependency dependency{0,VK_SUBPASS_EXTERNAL,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,0};
            VkRenderPassCreateInfo passInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};passInfo.attachmentCount=2;passInfo.pAttachments=attachments;passInfo.subpassCount=1;passInfo.pSubpasses=&subpass;passInfo.dependencyCount=1;passInfo.pDependencies=&dependency;
            VkRenderPass pass{};OK(CALL(vkCreateRenderPass)(c.device,&passInfo,nullptr,&pass));c.cleanup.push_back([&c,pass]{CALL(vkDestroyRenderPass)(c.device,pass,nullptr);});
            VkImageView views[]{color.view,depth.view};VkFramebufferCreateInfo frameInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};frameInfo.renderPass=pass;frameInfo.attachmentCount=2;frameInfo.pAttachments=views;frameInfo.width=frameInfo.height=64;frameInfo.layers=1;
            VkFramebuffer frame{};OK(CALL(vkCreateFramebuffer)(c.device,&frameInfo,nullptr,&frame));c.cleanup.push_back([&c,frame]{CALL(vkDestroyFramebuffer)(c.device,frame,nullptr);});
            VkPipelineShaderStageCreateInfo stages[2]{};stages[0].sType=stages[1].sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;stages[0].module=vs;stages[0].pName=production?"rawVertexMain":"vertexMain";stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;stages[1].module=ps;stages[1].pName=production?"main":"fragmentMain";
            VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
            VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            VkViewport viewport{0,0,64,64,0,1};VkRect2D scissor{{0,0},{64,64}};
            VkPipelineViewportStateCreateInfo viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};viewportState.viewportCount=viewportState.scissorCount=1;viewportState.pViewports=&viewport;viewportState.pScissors=&scissor;
            VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};raster.polygonMode=VK_POLYGON_MODE_FILL;raster.cullMode=VK_CULL_MODE_NONE;raster.lineWidth=1;
            VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
            VkPipelineDepthStencilStateCreateInfo depthState{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};depthState.depthTestEnable=VK_TRUE;depthState.depthWriteEnable=test.depthWrite;depthState.depthCompareOp=VK_COMPARE_OP_LESS;
            VkPipelineColorBlendAttachmentState blend{};blend.blendEnable=test.additive;
            blend.srcColorBlendFactor=blend.dstColorBlendFactor=blend.srcAlphaBlendFactor=blend.dstAlphaBlendFactor=VK_BLEND_FACTOR_ONE;
            blend.colorBlendOp=blend.alphaBlendOp=VK_BLEND_OP_ADD;blend.colorWriteMask=test.alphaWrite?15u:7u;
            VkPipelineColorBlendStateCreateInfo blendState{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};blendState.attachmentCount=1;blendState.pAttachments=&blend;
            VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};pipelineInfo.stageCount=2;pipelineInfo.pStages=stages;pipelineInfo.pVertexInputState=&input;pipelineInfo.pInputAssemblyState=&assembly;pipelineInfo.pViewportState=&viewportState;pipelineInfo.pRasterizationState=&raster;pipelineInfo.pMultisampleState=&samples;pipelineInfo.pDepthStencilState=&depthState;pipelineInfo.pColorBlendState=&blendState;pipelineInfo.layout=layout;pipelineInfo.renderPass=pass;
            VkPipeline pipeline{};OK(CALL(vkCreateGraphicsPipelines)(c.device,VK_NULL_HANDLE,1,&pipelineInfo,nullptr,&pipeline));c.cleanup.push_back([&c,pipeline]{CALL(vkDestroyPipeline)(c.device,pipeline,nullptr);});
            VkDescriptorImageInfo images[3]{{VK_NULL_HANDLE,test.srgb?srgbViews[0]:textureImages[0].view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},{VK_NULL_HANDLE,test.srgb?srgbViews[1]:textureImages[1].view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},{sampler,VK_NULL_HANDLE,VK_IMAGE_LAYOUT_UNDEFINED}};
            VkWriteDescriptorSet writes[3]{};for(uint32_t i=0;i<3;++i){writes[i].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;writes[i].dstSet=set;writes[i].dstBinding=production?(i==1?3u:i==2?1u:0u):i;writes[i].descriptorCount=1;writes[i].descriptorType=i==2?VK_DESCRIPTOR_TYPE_SAMPLER:VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;writes[i].pImageInfo=&images[i];}
            CALL(vkUpdateDescriptorSets)(c.device,3,writes,0,nullptr);
            VkClearValue clears[2]{};clears[0].color={{16.f/255,32.f/255,48.f/255,64.f/255}};clears[1].depthStencil={test.clearDepth,0};
            VkRenderPassBeginInfo passBegin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};passBegin.renderPass=pass;passBegin.framebuffer=frame;passBegin.renderArea=scissor;passBegin.clearValueCount=2;passBegin.pClearValues=clears;
            begin();CALL(vkCmdBeginRenderPass)(command,&passBegin,VK_SUBPASS_CONTENTS_INLINE);
            CALL(vkCmdBindPipeline)(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);
            CALL(vkCmdBindDescriptorSets)(command,VK_PIPELINE_BIND_POINT_GRAPHICS,layout,0,1,&set,0,nullptr);
            float parameters[]{1,0,0,test.shift0,0,1,0,0,1,0,0,test.shift1,0,1,0,0,test.second==2?.3f:.5f,0,0,0};
            auto emit=[&]{
                CALL(vkCmdPushConstants)(command,layout,VK_SHADER_STAGE_VERTEX_BIT,0,80,parameters);
                if(production){
                    SceneGpuUploadCandidate scene;scene.vertices.resize(3);scene.indices={0,1,2};scene.materials={{0,0}};scene.opaque={{0,0,0,3,0,0}};
                    VulkanPreparedSceneResourcesCandidate resources{};resources.vertices=vertexBuffer.buffer;resources.indices=indexBuffer.buffer;resources.vertexCapacity=3*sizeof(SceneGpuVertexCandidate);resources.indexCapacity=12;
                    resources.opaque=resources.translucent=pipeline;resources.layout=layout;resources.materials={set};resources.materialPipelines={pipeline};resources.twoTextureSlots={1};
                    resources.baseTextureTransform={{1,0,parameters[3],0,0,1,0,0}};resources.secondTextureTransform={{1,0,parameters[11],0,0,1,0,0}};
                    VulkanSceneCommandFunctionsCandidate functions{CALL(vkCmdBindPipeline),CALL(vkCmdBindDescriptorSets),CALL(vkCmdBindVertexBuffers),CALL(vkCmdBindIndexBuffer),CALL(vkCmdDrawIndexed)};
                    recordVulkanPreparedSceneCandidate(functions,CALL(vkCmdPushConstants),command,scene,RenderCameraCandidate{},{1},resources);
                }else CALL(vkCmdDraw)(command,3,1,0,0);
            };emit();
            if(test.second){parameters[11]=.5f;parameters[16]=test.second==2?.5f:.75f;emit();}
            CALL(vkCmdEndRenderPass)(command);
            VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={64,64,1};
            CALL(vkCmdCopyImageToBuffer)(command,color.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,readback.buffer,1,&copy);
            VkBufferMemoryBarrier host{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;host.srcQueueFamilyIndex=host.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;host.buffer=readback.buffer;host.size=VK_WHOLE_SIZE;
            CALL(vkCmdPipelineBarrier)(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,nullptr,1,&host,0,nullptr);submit();
            OK(CALL(vkMapMemory)(c.device,readback.memory,0,64*64*4,0,&mapped));std::vector<uint8_t> rgba(64*64*4);std::memcpy(rgba.data(),mapped,rgba.size());CALL(vkUnmapMemory)(c.device,readback.memory);
            auto pixel=rgba.data()+(32*64+32)*4;receipt<<test.name;for(size_t i=0;i<4;++i)receipt<<','<<unsigned(pixel[i]);receipt<<'\n';
            std::ofstream capture(output/(std::string(test.name)+".rgba"),std::ios::binary);capture.write(reinterpret_cast<const char*>(rgba.data()),std::streamsize(rgba.size()));if(!capture)throw std::runtime_error("capture write failed");
            std::cout<<test.name<<" RGBA="<<unsigned(pixel[0])<<','<<unsigned(pixel[1])<<','<<unsigned(pixel[2])<<','<<unsigned(pixel[3])<<'\n';
        }
        if(production)std::cout<<"production_fragment_and_shared_indexed_recorder=passed\n";receipt.flush();if(!receipt)throw std::runtime_error("receipt write failed");std::cout<<"PASS: 14 Vulkan GPU captures\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
