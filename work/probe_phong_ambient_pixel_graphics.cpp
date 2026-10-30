// Headless owned x64 Vulkan graphics fixture. No SFM process/window access.
// Controlled Phong ambient pixel branch; float textures/target. Test only.
#define NOMINMAX
#define VK_NO_PROTOTYPES
#include <windows.h>
#include <vulkan/vulkan.h>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

struct PhongVkContext {
    HMODULE library=nullptr;VkInstance instance{};VkDevice device{};VkPhysicalDevice physical{};VkQueue queue{};uint32_t family=0;
    VkPhysicalDeviceMemoryProperties memory{};VkDebugUtilsMessengerEXT messenger{};bool validation=false;
    unsigned*errors=nullptr,*warnings=nullptr;std::ofstream validationLog;std::vector<std::function<void()>>cleanup;
    template<class T>T load(const char*name){const auto p=GetProcAddress(library,name);if(!p)throw std::runtime_error(name);return reinterpret_cast<T>(p);}
    static void check(VkResult result){if(result!=VK_SUCCESS)throw std::runtime_error("Vulkan result="+std::to_string(result));}
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug(VkDebugUtilsMessageSeverityFlagBitsEXT severity,VkDebugUtilsMessageTypeFlagsEXT,const VkDebugUtilsMessengerCallbackDataEXT*data,void*user){
        auto&c=*static_cast<PhongVkContext*>(user);if(severity&VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)++*c.errors;
        if(severity&VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)++*c.warnings;
        c.validationLog<<(data&&data->pMessage?data->pMessage:"missing validation message")<<'\n';return VK_FALSE;
    }
    ~PhongVkContext(){
        if(device){load<PFN_vkDeviceWaitIdle>("vkDeviceWaitIdle")(device);for(auto it=cleanup.rbegin();it!=cleanup.rend();++it)(*it)();load<PFN_vkDestroyDevice>("vkDestroyDevice")(device,nullptr);}
        if(messenger){const auto get=load<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(get(instance,"vkDestroyDebugUtilsMessengerEXT"))(instance,messenger,nullptr);}
        if(instance)load<PFN_vkDestroyInstance>("vkDestroyInstance")(instance,nullptr);if(library)FreeLibrary(library);
    }
    uint32_t memoryType(uint32_t bits,VkMemoryPropertyFlags flags){for(uint32_t i=0;i<memory.memoryTypeCount;++i)if((bits&(1u<<i))&&(memory.memoryTypes[i].propertyFlags&flags)==flags)return i;throw std::runtime_error("Vulkan memory type unavailable");}
};
#define VC(name) c.load<PFN_##name>(#name)
#define VKCHECK(expression) PhongVkContext::check(expression)
struct PhongVkBuffer{VkBuffer handle{};VkDeviceMemory memory{};void*mapped=nullptr;};
static PhongVkBuffer makePhongBuffer(PhongVkContext&c,VkDeviceSize size,VkBufferUsageFlags usage){
    PhongVkBuffer out;VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};info.size=size;info.usage=usage;VKCHECK(VC(vkCreateBuffer)(c.device,&info,nullptr,&out.handle));
    VkMemoryRequirements needs{};VC(vkGetBufferMemoryRequirements)(c.device,out.handle,&needs);VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=needs.size;allocation.memoryTypeIndex=c.memoryType(needs.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VKCHECK(VC(vkAllocateMemory)(c.device,&allocation,nullptr,&out.memory));c.cleanup.push_back([&c,v=out.memory]{VC(vkFreeMemory)(c.device,v,nullptr);});c.cleanup.push_back([&c,v=out.handle]{VC(vkDestroyBuffer)(c.device,v,nullptr);});
    VKCHECK(VC(vkBindBufferMemory)(c.device,out.handle,out.memory,0));VKCHECK(VC(vkMapMemory)(c.device,out.memory,0,size,0,&out.mapped));c.cleanup.push_back([&c,v=out.memory]{VC(vkUnmapMemory)(c.device,v);});return out;
}
struct PhongVkImage{VkImage handle{};VkImageView view{};VkDeviceMemory memory{};};
static PhongVkImage makePhongImage(PhongVkContext&c,uint32_t size,VkImageUsageFlags usage){
    PhongVkImage out;VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};info.imageType=VK_IMAGE_TYPE_2D;info.format=VK_FORMAT_R32G32B32A32_SFLOAT;info.extent={size,size,1};info.mipLevels=info.arrayLayers=1;info.samples=VK_SAMPLE_COUNT_1_BIT;info.tiling=VK_IMAGE_TILING_OPTIMAL;info.usage=usage;
    VKCHECK(VC(vkCreateImage)(c.device,&info,nullptr,&out.handle));VkMemoryRequirements needs{};VC(vkGetImageMemoryRequirements)(c.device,out.handle,&needs);VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=needs.size;allocation.memoryTypeIndex=c.memoryType(needs.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VKCHECK(VC(vkAllocateMemory)(c.device,&allocation,nullptr,&out.memory));c.cleanup.push_back([&c,v=out.memory]{VC(vkFreeMemory)(c.device,v,nullptr);});c.cleanup.push_back([&c,v=out.handle]{VC(vkDestroyImage)(c.device,v,nullptr);});VKCHECK(VC(vkBindImageMemory)(c.device,out.handle,out.memory,0));
    VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};view.image=out.handle;view.viewType=VK_IMAGE_VIEW_TYPE_2D;view.format=info.format;view.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};VKCHECK(VC(vkCreateImageView)(c.device,&view,nullptr,&out.view));c.cleanup.push_back([&c,v=out.view]{VC(vkDestroyImageView)(c.device,v,nullptr);});return out;
}
static VkShaderModule phongModule(PhongVkContext&c,const std::filesystem::path&path){
    std::ifstream in(path,std::ios::binary|std::ios::ate);if(!in)throw std::runtime_error("owned SPIR-V missing");auto size=in.tellg();if(size<20||uint64_t(size)%4||uint64_t(size)>1048576)throw std::runtime_error("SPIR-V framing");std::vector<uint32_t>words(size_t(size)/4);in.seekg(0);in.read(reinterpret_cast<char*>(words.data()),size);if(!in||words[0]!=0x07230203)throw std::runtime_error("SPIR-V read/magic");
    VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};info.codeSize=size_t(size);info.pCode=words.data();VkShaderModule out{};VKCHECK(VC(vkCreateShaderModule)(c.device,&info,nullptr,&out));c.cleanup.push_back([&c,out]{VC(vkDestroyShaderModule)(c.device,out,nullptr);});return out;
}
#include "phong_ambient_pixel_fixture.h"
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"native x64 required");
    if(argc!=6)throw std::runtime_error("usage: shader-dir query-file fresh-dir vendor device");
    const auto queries=phongAmbientPixelQueries(argv[2]);
    const std::filesystem::path output=argv[3];
    if(!std::filesystem::create_directory(output))throw std::runtime_error("fresh Vulkan directory required");
    const auto vendor=uint32_t(std::stoul(argv[4])),deviceId=uint32_t(std::stoul(argv[5]));
    unsigned errors=0,warnings=0;size_t cases=0;bool validationEnabled=false;
    {
        PhongVkContext c;c.errors=&errors;c.warnings=&warnings;c.validationLog.open(output/"validation.log");c.library=LoadLibraryExW(L"vulkan-1.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);if(!c.library)throw std::runtime_error("Vulkan loader missing");
        uint32_t n=0;VKCHECK(VC(vkEnumerateInstanceLayerProperties)(&n,nullptr));std::vector<VkLayerProperties>layers(n);VKCHECK(VC(vkEnumerateInstanceLayerProperties)(&n,layers.data()));bool layer=false;for(const auto&p:layers)layer|=std::string(p.layerName)=="VK_LAYER_KHRONOS_validation";
        VKCHECK(VC(vkEnumerateInstanceExtensionProperties)(nullptr,&n,nullptr));std::vector<VkExtensionProperties>extensions(n);VKCHECK(VC(vkEnumerateInstanceExtensionProperties)(nullptr,&n,extensions.data()));bool debug=false;for(const auto&p:extensions)debug|=std::string(p.extensionName)==VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        c.validation=validationEnabled=layer&&debug;const char*layerName="VK_LAYER_KHRONOS_validation",*debugName=VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        VkDebugUtilsMessengerCreateInfoEXT debugInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};debugInfo.messageSeverity=VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;debugInfo.messageType=VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;debugInfo.pfnUserCallback=PhongVkContext::debug;debugInfo.pUserData=&c;
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.pApplicationName="SFM64 bounded Phong pixel fixture";app.apiVersion=VK_API_VERSION_1_0;
        VkInstanceCreateInfo instanceInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};instanceInfo.pApplicationInfo=&app;if(c.validation){instanceInfo.enabledLayerCount=instanceInfo.enabledExtensionCount=1;instanceInfo.ppEnabledLayerNames=&layerName;instanceInfo.ppEnabledExtensionNames=&debugName;instanceInfo.pNext=&debugInfo;}
        VKCHECK(VC(vkCreateInstance)(&instanceInfo,nullptr,&c.instance));if(c.validation){auto get=VC(vkGetInstanceProcAddr);auto create=reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(get(c.instance,"vkCreateDebugUtilsMessengerEXT"));if(!create)throw std::runtime_error("debug messenger function");VKCHECK(create(c.instance,&debugInfo,nullptr,&c.messenger));}
        VKCHECK(VC(vkEnumeratePhysicalDevices)(c.instance,&n,nullptr));std::vector<VkPhysicalDevice>physical(n);VKCHECK(VC(vkEnumeratePhysicalDevices)(c.instance,&n,physical.data()));VkPhysicalDeviceProperties properties{};
        for(auto candidate:physical){VkPhysicalDeviceProperties p{};VC(vkGetPhysicalDeviceProperties)(candidate,&p);if(p.vendorID!=vendor||p.deviceID!=deviceId)continue;uint32_t count=0;VC(vkGetPhysicalDeviceQueueFamilyProperties)(candidate,&count,nullptr);std::vector<VkQueueFamilyProperties>queues(count);VC(vkGetPhysicalDeviceQueueFamilyProperties)(candidate,&count,queues.data());for(uint32_t i=0;i<count;++i)if(queues[i].queueCount&&(queues[i].queueFlags&VK_QUEUE_GRAPHICS_BIT)){c.physical=candidate;c.family=i;properties=p;break;}if(c.physical)break;}
        if(!c.physical)throw std::runtime_error("same D3D9 reference adapter unavailable in Vulkan");VkFormatProperties fp{};VC(vkGetPhysicalDeviceFormatProperties)(c.physical,VK_FORMAT_R32G32B32A32_SFLOAT,&fp);
        const auto needed=VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
        if((fp.optimalTilingFeatures&needed)!=needed)throw std::runtime_error("float color/sampling unavailable");
        std::ofstream deviceInfo(output/"device.txt");deviceInfo<<"description="<<properties.deviceName<<"\nvendor="<<properties.vendorID<<"\ndevice="<<properties.deviceID<<"\napi_version_raw="<<properties.apiVersion<<"\ndriver_version_raw="<<properties.driverVersion<<"\nvalidation_enabled="<<c.validation<<"\nwindow_created=0\n";deviceInfo.flush();if(!deviceInfo)throw std::runtime_error("Vulkan metadata write");std::cout<<"adapter="<<properties.deviceName<<" validation="<<c.validation<<" headless=1 x64=1\n"<<std::flush;
        float priority=1;VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};queueInfo.queueFamilyIndex=c.family;queueInfo.queueCount=1;queueInfo.pQueuePriorities=&priority;VkDeviceCreateInfo devInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};devInfo.queueCreateInfoCount=1;devInfo.pQueueCreateInfos=&queueInfo;VKCHECK(VC(vkCreateDevice)(c.physical,&devInfo,nullptr,&c.device));VC(vkGetDeviceQueue)(c.device,c.family,0,&c.queue);VC(vkGetPhysicalDeviceMemoryProperties)(c.physical,&c.memory);
        VkCommandPool pool{};VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};poolInfo.queueFamilyIndex=c.family;poolInfo.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;VKCHECK(VC(vkCreateCommandPool)(c.device,&poolInfo,nullptr,&pool));c.cleanup.push_back([&c,pool]{VC(vkDestroyCommandPool)(c.device,pool,nullptr);});VkCommandBuffer command{};VkCommandBufferAllocateInfo cmdInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};cmdInfo.commandPool=pool;cmdInfo.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;cmdInfo.commandBufferCount=1;VKCHECK(VC(vkAllocateCommandBuffers)(c.device,&cmdInfo,&command));
        auto input=makePhongBuffer(c,sizeof(PhongAmbientPixelQuery),VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        auto staging=makePhongBuffer(c,32,VK_BUFFER_USAGE_TRANSFER_SRC_BIT),readback=makePhongBuffer(c,256,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        auto target=makePhongImage(c,4,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        std::array<PhongVkImage,2>textures;for(auto&i:textures)i=makePhongImage(c,1,VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        VkSamplerCreateInfo samplerInfo{};samplerInfo.sType=VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;samplerInfo.magFilter=samplerInfo.minFilter=VK_FILTER_NEAREST;samplerInfo.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.addressModeU=samplerInfo.addressModeV=samplerInfo.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;samplerInfo.maxLod=0;
        VkSampler sampler{};VKCHECK(VC(vkCreateSampler)(c.device,&samplerInfo,nullptr,&sampler));c.cleanup.push_back([&c,sampler]{VC(vkDestroySampler)(c.device,sampler,nullptr);});
        const VkDescriptorSetLayoutBinding bindings[]={{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
            {1,VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},{2,VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
            {3,VK_DESCRIPTOR_TYPE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}};
        VkDescriptorSetLayoutCreateInfo setInfo{};setInfo.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;setInfo.bindingCount=4;setInfo.pBindings=bindings;
        VkDescriptorSetLayout setLayout{};VKCHECK(VC(vkCreateDescriptorSetLayout)(c.device,&setInfo,nullptr,&setLayout));c.cleanup.push_back([&c,setLayout]{VC(vkDestroyDescriptorSetLayout)(c.device,setLayout,nullptr);});
        const VkDescriptorPoolSize sizes[]={{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1},{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,2},{VK_DESCRIPTOR_TYPE_SAMPLER,1}};
        VkDescriptorPoolCreateInfo descInfo{};descInfo.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;descInfo.maxSets=1;descInfo.poolSizeCount=3;descInfo.pPoolSizes=sizes;
        VkDescriptorPool descPool{};VKCHECK(VC(vkCreateDescriptorPool)(c.device,&descInfo,nullptr,&descPool));c.cleanup.push_back([&c,descPool]{VC(vkDestroyDescriptorPool)(c.device,descPool,nullptr);});
        VkDescriptorSetAllocateInfo sa{};sa.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;sa.descriptorPool=descPool;sa.descriptorSetCount=1;sa.pSetLayouts=&setLayout;
        VkDescriptorSet set{};VKCHECK(VC(vkAllocateDescriptorSets)(c.device,&sa,&set));VkDescriptorBufferInfo bufferInfo{input.handle,0,sizeof(PhongAmbientPixelQuery)};
        const VkDescriptorImageInfo images[]={{VK_NULL_HANDLE,textures[0].view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},{VK_NULL_HANDLE,textures[1].view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},{sampler,VK_NULL_HANDLE,VK_IMAGE_LAYOUT_UNDEFINED}};
        std::array<VkWriteDescriptorSet,4>writes{};
        for(unsigned i=0;i<4;++i){writes[i].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;writes[i].dstSet=set;writes[i].dstBinding=i;writes[i].descriptorCount=1;writes[i].descriptorType=bindings[i].descriptorType;if(i)writes[i].pImageInfo=&images[i-1];else writes[i].pBufferInfo=&bufferInfo;}
        VC(vkUpdateDescriptorSets)(c.device,4,writes.data(),0,nullptr);
        VkPipelineLayoutCreateInfo layoutInfo{};layoutInfo.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;layoutInfo.setLayoutCount=1;layoutInfo.pSetLayouts=&setLayout;
        VkPipelineLayout layout{};VKCHECK(VC(vkCreatePipelineLayout)(c.device,&layoutInfo,nullptr,&layout));c.cleanup.push_back([&c,layout]{VC(vkDestroyPipelineLayout)(c.device,layout,nullptr);});
        VkAttachmentDescription attachment{};attachment.format=VK_FORMAT_R32G32B32A32_SFLOAT;attachment.samples=VK_SAMPLE_COUNT_1_BIT;attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;attachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;attachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;attachment.finalLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        VkAttachmentReference ref{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;sub.colorAttachmentCount=1;sub.pColorAttachments=&ref;
        const VkSubpassDependency deps[]={{VK_SUBPASS_EXTERNAL,0,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,0,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,0},
            {0,VK_SUBPASS_EXTERNAL,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,0}};
        VkRenderPassCreateInfo rp{};rp.sType=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;rp.attachmentCount=1;rp.pAttachments=&attachment;rp.subpassCount=1;rp.pSubpasses=&sub;rp.dependencyCount=2;rp.pDependencies=deps;
        VkRenderPass pass{};VKCHECK(VC(vkCreateRenderPass)(c.device,&rp,nullptr,&pass));c.cleanup.push_back([&c,pass]{VC(vkDestroyRenderPass)(c.device,pass,nullptr);});
        VkFramebufferCreateInfo fb{};fb.sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;fb.renderPass=pass;fb.attachmentCount=1;fb.pAttachments=&target.view;fb.width=fb.height=4;fb.layers=1;
        VkFramebuffer framebuffer{};VKCHECK(VC(vkCreateFramebuffer)(c.device,&fb,nullptr,&framebuffer));c.cleanup.push_back([&c,framebuffer]{VC(vkDestroyFramebuffer)(c.device,framebuffer,nullptr);});
        VkPipelineShaderStageCreateInfo stages[2]{};
        for(auto&s:stages)s.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;stages[0].module=phongModule(c,std::filesystem::path(argv[1])/"owned-phong-vertex.spv");stages[0].pName="vertexMain";
        stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;stages[1].module=phongModule(c,std::filesystem::path(argv[1])/"owned-phong-pixel.spv");stages[1].pName="pixelMain";
        VkPipelineVertexInputStateCreateInfo vi{};vi.sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        VkPipelineInputAssemblyStateCreateInfo ia{};ia.sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;ia.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        const VkViewport viewport{0,0,4,4,0,1};const VkRect2D scissor{{0,0},{4,4}};
        VkPipelineViewportStateCreateInfo vp{};vp.sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;vp.viewportCount=vp.scissorCount=1;vp.pViewports=&viewport;vp.pScissors=&scissor;
        VkPipelineRasterizationStateCreateInfo rs{};rs.sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;rs.polygonMode=VK_POLYGON_MODE_FILL;rs.cullMode=VK_CULL_MODE_NONE;rs.lineWidth=1;
        VkPipelineMultisampleStateCreateInfo ms{};ms.sType=VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkPipelineColorBlendAttachmentState blend{};blend.colorWriteMask=15;
        VkPipelineColorBlendStateCreateInfo bs{};bs.sType=VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;bs.attachmentCount=1;bs.pAttachments=&blend;
        VkGraphicsPipelineCreateInfo pi{};pi.sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;pi.stageCount=2;pi.pStages=stages;pi.pVertexInputState=&vi;pi.pInputAssemblyState=&ia;pi.pViewportState=&vp;pi.pRasterizationState=&rs;pi.pMultisampleState=&ms;pi.pColorBlendState=&bs;pi.layout=layout;pi.renderPass=pass;
        VkPipeline pipeline{};VKCHECK(VC(vkCreateGraphicsPipelines)(c.device,VK_NULL_HANDLE,1,&pi,nullptr,&pipeline));c.cleanup.push_back([&c,pipeline]{VC(vkDestroyPipeline)(c.device,pipeline,nullptr);});
        std::ofstream samples(output/"vulkan-pixels.bin",std::ios::binary);
        for(const auto&q:queries){
            std::memcpy(input.mapped,&q,sizeof(q));std::memcpy(staging.mapped,q.base.data(),16);std::memcpy(static_cast<uint8_t*>(staging.mapped)+16,q.normalSample.data(),16);
            VKCHECK(VC(vkResetCommandBuffer)(command,0));VkCommandBufferBeginInfo begin{};begin.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;VKCHECK(VC(vkBeginCommandBuffer)(command,&begin));
            VkMemoryBarrier host{};host.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;host.srcAccessMask=VK_ACCESS_HOST_WRITE_BIT;host.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_TRANSFER_READ_BIT;
            VC(vkCmdPipelineBarrier)(command,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_TRANSFER_BIT,0,1,&host,0,nullptr,0,nullptr);
            std::array<VkImageMemoryBarrier,2>transitions{};
            for(unsigned i=0;i<2;++i){auto&bar=transitions[i];bar.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;bar.srcAccessMask=cases?VK_ACCESS_SHADER_READ_BIT:0;bar.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
                bar.oldLayout=cases?VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:VK_IMAGE_LAYOUT_UNDEFINED;bar.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;bar.srcQueueFamilyIndex=bar.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;bar.image=textures[i].handle;bar.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};}
            VC(vkCmdPipelineBarrier)(command,cases?VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT:VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,2,transitions.data());
            for(unsigned i=0;i<2;++i){VkBufferImageCopy copy{};copy.bufferOffset=i*16;copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={1,1,1};VC(vkCmdCopyBufferToImage)(command,staging.handle,textures[i].handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
                transitions[i].srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;transitions[i].dstAccessMask=VK_ACCESS_SHADER_READ_BIT;transitions[i].oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;transitions[i].newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;}
            VC(vkCmdPipelineBarrier)(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,0,nullptr,2,transitions.data());
            VkClearValue clear{};VkRenderPassBeginInfo render{};render.sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;render.renderPass=pass;render.framebuffer=framebuffer;render.renderArea={{0,0},{4,4}};render.clearValueCount=1;render.pClearValues=&clear;
            VC(vkCmdBeginRenderPass)(command,&render,VK_SUBPASS_CONTENTS_INLINE);VC(vkCmdBindPipeline)(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);VC(vkCmdBindDescriptorSets)(command,VK_PIPELINE_BIND_POINT_GRAPHICS,layout,0,1,&set,0,nullptr);VC(vkCmdDraw)(command,6,1,0,0);VC(vkCmdEndRenderPass)(command);
            VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={4,4,1};VC(vkCmdCopyImageToBuffer)(command,target.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,readback.handle,1,&copy);
            VkBufferMemoryBarrier readable{};readable.sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;readable.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;readable.dstAccessMask=VK_ACCESS_HOST_READ_BIT;readable.srcQueueFamilyIndex=readable.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;readable.buffer=readback.handle;readable.size=VK_WHOLE_SIZE;
            VC(vkCmdPipelineBarrier)(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,nullptr,1,&readable,0,nullptr);VKCHECK(VC(vkEndCommandBuffer)(command));
            VkSubmitInfo submit{};submit.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;submit.commandBufferCount=1;submit.pCommandBuffers=&command;VKCHECK(VC(vkQueueSubmit)(c.queue,1,&submit,VK_NULL_HANDLE));VKCHECK(VC(vkQueueWaitIdle)(c.queue));
            std::array<float,64>pixels{};std::memcpy(pixels.data(),readback.mapped,256);
            for(unsigned i=0;i<16;++i)for(unsigned channel=0;channel<4;++channel)if(!std::isfinite(pixels[i*4+channel])||pixels[i*4+channel]!=pixels[channel])throw std::runtime_error("Vulkan constant quad mismatch");
            samples.write(reinterpret_cast<const char*>(pixels.data()),256);++cases;
        }
        samples.flush();if(!samples)throw std::runtime_error("Vulkan result write");VKCHECK(VC(vkDeviceWaitIdle)(c.device));
    }
    std::ofstream status(output/"validation-status.txt");status<<"enabled="<<validationEnabled<<"\nerrors="<<errors<<"\nwarnings="<<warnings<<"\n";status.flush();if(!status)throw std::runtime_error("validation metadata");
    if(errors)throw std::runtime_error("Vulkan validation errors="+std::to_string(errors));
    std::cout<<"PASS graphics_cases="<<cases<<" pixels_per_case=16 validation_enabled="<<validationEnabled<<" warnings="<<warnings<<" errors="<<errors<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
