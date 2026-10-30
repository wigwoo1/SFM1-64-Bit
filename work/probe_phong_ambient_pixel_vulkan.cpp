#define NOMINMAX
#define VK_NO_PROTOTYPES
#include <windows.h>
#include <vulkan/vulkan.h>
#include "phong_ambient_pixel_fixture.h"
#include <functional>
#include <iomanip>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>

// Isolated owned Phong arithmetic compute test. Already sampled inputs only.
// No Vulkan texture/interpolation proof, SFM process, original DLL or engine.
struct AmbientGpuContext {
    HMODULE library=nullptr;VkInstance instance{};VkDevice device{};VkPhysicalDevice physical{};VkQueue queue{};uint32_t family=0;
    VkPhysicalDeviceMemoryProperties memory{};std::vector<std::function<void()>>cleanup;
    template<class T>T load(const char*name){auto p=GetProcAddress(library,name);if(!p)throw std::runtime_error(name);return reinterpret_cast<T>(p);}
    static void check(VkResult r){if(r!=VK_SUCCESS)throw std::runtime_error("ambient Vulkan result="+std::to_string(r));}
    ~AmbientGpuContext(){if(device){load<PFN_vkDeviceWaitIdle>("vkDeviceWaitIdle")(device);for(auto i=cleanup.rbegin();i!=cleanup.rend();++i)(*i)();load<PFN_vkDestroyDevice>("vkDestroyDevice")(device,nullptr);}if(instance)load<PFN_vkDestroyInstance>("vkDestroyInstance")(instance,nullptr);if(library)FreeLibrary(library);}
};
#define FN(name) c.load<PFN_##name>(#name)
#define CHECK(value) AmbientGpuContext::check(value)
struct AmbientGpuBuffer {VkBuffer buffer{};VkDeviceMemory memory{};VkDeviceSize bytes=0;};
AmbientGpuBuffer ambientGpuBuffer(AmbientGpuContext&c,VkDeviceSize bytes,const void*data){
    AmbientGpuBuffer result;result.bytes=bytes;
    VkBufferCreateInfo bi{};bi.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;bi.size=bytes;bi.usage=VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    CHECK(FN(vkCreateBuffer)(c.device,&bi,nullptr,&result.buffer));
    VkMemoryRequirements requirements{};FN(vkGetBufferMemoryRequirements)(c.device,result.buffer,&requirements);
    uint32_t type=UINT32_MAX;for(uint32_t i=0;i<c.memory.memoryTypeCount;++i)if((requirements.memoryTypeBits&(1u<<i))&&(c.memory.memoryTypes[i].propertyFlags&(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))==(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){type=i;break;}
    if(type==UINT32_MAX){FN(vkDestroyBuffer)(c.device,result.buffer,nullptr);throw std::runtime_error("coherent ambient memory unavailable");}
    VkMemoryAllocateInfo ai{};ai.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;ai.allocationSize=requirements.size;ai.memoryTypeIndex=type;
    const auto allocated=FN(vkAllocateMemory)(c.device,&ai,nullptr,&result.memory);
    if(allocated!=VK_SUCCESS){FN(vkDestroyBuffer)(c.device,result.buffer,nullptr);CHECK(allocated);}
    c.cleanup.push_back([&c,m=result.memory]{FN(vkFreeMemory)(c.device,m,nullptr);});
    c.cleanup.push_back([&c,b=result.buffer]{FN(vkDestroyBuffer)(c.device,b,nullptr);});
    CHECK(FN(vkBindBufferMemory)(c.device,result.buffer,result.memory,0));
    void*mapped=nullptr;CHECK(FN(vkMapMemory)(c.device,result.memory,0,bytes,0,&mapped));
    if(data)std::memcpy(mapped,data,size_t(bytes));else std::memset(mapped,0,size_t(bytes));FN(vkUnmapMemory)(c.device,result.memory);return result;
}
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"Native x64 required");
    if(argc!=4)throw std::runtime_error("usage: probe shader.spv records.bin fresh-results.bin");
    if(std::filesystem::exists(argv[3]))throw std::runtime_error("fresh workspace output required");
    auto read=[](const char*path){std::ifstream f(path,std::ios::binary|std::ios::ate);if(!f)throw std::runtime_error("input missing");auto n=f.tellg();if(n<=0||uint64_t(n)>64*1024*1024)throw std::runtime_error("input size");std::vector<uint8_t>b(static_cast<size_t>(n));f.seekg(0);f.read(reinterpret_cast<char*>(b.data()),n);if(!f)throw std::runtime_error("input read");return b;};
    const auto records=phongAmbientPixelQueries(argv[2]);
    std::vector<uint8_t>bytes(records.size()*sizeof(PhongAmbientPixelQuery));std::memcpy(bytes.data(),records.data(),bytes.size());
    AmbientGpuContext c;c.library=LoadLibraryExW(L"vulkan-1.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);if(!c.library)throw std::runtime_error("Vulkan loader missing");
    VkInstanceCreateInfo ii{};ii.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;CHECK(FN(vkCreateInstance)(&ii,nullptr,&c.instance));
    uint32_t count=0;CHECK(FN(vkEnumeratePhysicalDevices)(c.instance,&count,nullptr));std::vector<VkPhysicalDevice>devices(count);CHECK(FN(vkEnumeratePhysicalDevices)(c.instance,&count,devices.data()));
    for(auto physical:devices){uint32_t n=0;FN(vkGetPhysicalDeviceQueueFamilyProperties)(physical,&n,nullptr);std::vector<VkQueueFamilyProperties>properties(n);FN(vkGetPhysicalDeviceQueueFamilyProperties)(physical,&n,properties.data());for(uint32_t i=0;i<n;++i)if(properties[i].queueCount&&(properties[i].queueFlags&VK_QUEUE_COMPUTE_BIT)){c.physical=physical;c.family=i;break;}if(c.physical)break;}
    if(!c.physical)throw std::runtime_error("Vulkan compute device missing");
    VkPhysicalDeviceProperties caps{};FN(vkGetPhysicalDeviceProperties)(c.physical,&caps);
    const uint32_t groups=uint32_t((records.size()+63)/64);const size_t outputBytes=records.size()*16;
    if(bytes.size()>caps.limits.maxStorageBufferRange||outputBytes>caps.limits.maxStorageBufferRange||groups>caps.limits.maxComputeWorkGroupCount[0])throw std::runtime_error("ambient GPU capacity");
    float priority=1;VkDeviceQueueCreateInfo qi{};qi.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;qi.queueFamilyIndex=c.family;qi.queueCount=1;qi.pQueuePriorities=&priority;
    VkDeviceCreateInfo di{};di.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;di.queueCreateInfoCount=1;di.pQueueCreateInfos=&qi;CHECK(FN(vkCreateDevice)(c.physical,&di,nullptr,&c.device));
    FN(vkGetDeviceQueue)(c.device,c.family,0,&c.queue);FN(vkGetPhysicalDeviceMemoryProperties)(c.physical,&c.memory);
    auto input=ambientGpuBuffer(c,bytes.size(),bytes.data()),output=ambientGpuBuffer(c,outputBytes,nullptr);
    VkDescriptorSetLayoutBinding bindings[2]{{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}};
    VkDescriptorSetLayoutCreateInfo si{};si.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;si.bindingCount=2;si.pBindings=bindings;VkDescriptorSetLayout setLayout{};CHECK(FN(vkCreateDescriptorSetLayout)(c.device,&si,nullptr,&setLayout));c.cleanup.push_back([&c,setLayout]{FN(vkDestroyDescriptorSetLayout)(c.device,setLayout,nullptr);});
    VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,2};VkDescriptorPoolCreateInfo pi{};pi.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;pi.maxSets=1;pi.poolSizeCount=1;pi.pPoolSizes=&poolSize;VkDescriptorPool pool{};CHECK(FN(vkCreateDescriptorPool)(c.device,&pi,nullptr,&pool));c.cleanup.push_back([&c,pool]{FN(vkDestroyDescriptorPool)(c.device,pool,nullptr);});
    VkDescriptorSetAllocateInfo sa{};sa.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;sa.descriptorPool=pool;sa.descriptorSetCount=1;sa.pSetLayouts=&setLayout;VkDescriptorSet set{};CHECK(FN(vkAllocateDescriptorSets)(c.device,&sa,&set));
    VkDescriptorBufferInfo buffers[2]{{input.buffer,0,input.bytes},{output.buffer,0,output.bytes}};VkWriteDescriptorSet writes[2]{};for(uint32_t i=0;i<2;++i){writes[i].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;writes[i].dstSet=set;writes[i].dstBinding=i;writes[i].descriptorCount=1;writes[i].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;writes[i].pBufferInfo=&buffers[i];}FN(vkUpdateDescriptorSets)(c.device,2,writes,0,nullptr);
    VkPipelineLayoutCreateInfo li{};li.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;li.setLayoutCount=1;li.pSetLayouts=&setLayout;VkPipelineLayout layout{};CHECK(FN(vkCreatePipelineLayout)(c.device,&li,nullptr,&layout));c.cleanup.push_back([&c,layout]{FN(vkDestroyPipelineLayout)(c.device,layout,nullptr);});
    auto shaderBytes=read(argv[1]);if(shaderBytes.size()%4)throw std::runtime_error("SPIR-V size");std::vector<uint32_t>words(shaderBytes.size()/4);std::memcpy(words.data(),shaderBytes.data(),shaderBytes.size());
    VkShaderModuleCreateInfo mi{};mi.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;mi.codeSize=shaderBytes.size();mi.pCode=words.data();VkShaderModule shader{};CHECK(FN(vkCreateShaderModule)(c.device,&mi,nullptr,&shader));c.cleanup.push_back([&c,shader]{FN(vkDestroyShaderModule)(c.device,shader,nullptr);});
    VkComputePipelineCreateInfo ci{};ci.sType=VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;ci.layout=layout;ci.stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;ci.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;ci.stage.module=shader;ci.stage.pName="main";VkPipeline pipeline{};CHECK(FN(vkCreateComputePipelines)(c.device,VK_NULL_HANDLE,1,&ci,nullptr,&pipeline));c.cleanup.push_back([&c,pipeline]{FN(vkDestroyPipeline)(c.device,pipeline,nullptr);});
    VkCommandPoolCreateInfo cp{};cp.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;cp.queueFamilyIndex=c.family;VkCommandPool commands{};CHECK(FN(vkCreateCommandPool)(c.device,&cp,nullptr,&commands));c.cleanup.push_back([&c,commands]{FN(vkDestroyCommandPool)(c.device,commands,nullptr);});
    VkCommandBufferAllocateInfo ca{};ca.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;ca.commandPool=commands;ca.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ca.commandBufferCount=1;VkCommandBuffer command{};CHECK(FN(vkAllocateCommandBuffers)(c.device,&ca,&command));
    VkCommandBufferBeginInfo bi{};bi.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;CHECK(FN(vkBeginCommandBuffer)(command,&bi));VkMemoryBarrier host{};host.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;host.srcAccessMask=VK_ACCESS_HOST_WRITE_BIT;host.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;FN(vkCmdPipelineBarrier)(command,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&host,0,nullptr,0,nullptr);FN(vkCmdBindPipeline)(command,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline);FN(vkCmdBindDescriptorSets)(command,VK_PIPELINE_BIND_POINT_COMPUTE,layout,0,1,&set,0,nullptr);FN(vkCmdDispatch)(command,groups,1,1);
    VkMemoryBarrier barrier{};barrier.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;barrier.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_HOST_READ_BIT;FN(vkCmdPipelineBarrier)(command,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&barrier,0,nullptr,0,nullptr);CHECK(FN(vkEndCommandBuffer)(command));
    VkSubmitInfo submit{};submit.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;submit.commandBufferCount=1;submit.pCommandBuffers=&command;CHECK(FN(vkQueueSubmit)(c.queue,1,&submit,VK_NULL_HANDLE));CHECK(FN(vkQueueWaitIdle)(c.queue));
    void*mapped=nullptr;CHECK(FN(vkMapMemory)(c.device,output.memory,0,output.bytes,0,&mapped));std::vector<std::array<float,4>>actual(records.size());std::memcpy(actual.data(),mapped,outputBytes);FN(vkUnmapMemory)(c.device,output.memory);
    for(const auto&v:actual)for(float component:v)if(!std::isfinite(component))throw std::runtime_error("nonfinite owned result");
    std::ofstream capture(argv[3],std::ios::binary);capture.write(reinterpret_cast<const char*>(actual.data()),std::streamsize(outputBytes));if(!capture)throw std::runtime_error("result output failed");
    std::cout<<"PASS adapter="<<caps.deviceName<<" vendor="<<caps.vendorID<<" device="<<caps.deviceID<<" driver="<<caps.driverVersion<<" x64=1 arithmetic_records="<<records.size()<<" sampled_inputs_only=1\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
