// Minimal Vulkan 1.0 ABI probe. No SDK install, rendering or shader dependency.
#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <string>
using Handle=void*;
struct InstanceInfo{uint32_t type;const void*next;uint32_t flags;const void*application;uint32_t layerCount;const char*const*layers;uint32_t extensionCount;const char*const*extensions;};
struct QueueInfo{uint32_t type;const void*next;uint32_t flags,family,count;const float*priorities;};
struct DeviceInfo{uint32_t type;const void*next;uint32_t flags,queueCount;const QueueInfo*queues;uint32_t layerCount;const char*const*layers;uint32_t extensionCount;const char*const*extensions;const void*features;};
struct QueueProperties{uint32_t flags,count,timestampBits,granularity[3];};
using CreateInstance=int32_t(WINAPI*)(const InstanceInfo*,const void*,Handle*);
using DestroyInstance=void(WINAPI*)(Handle,const void*);
using EnumerateDevices=int32_t(WINAPI*)(Handle,uint32_t*,Handle*);
using QueueFamilies=void(WINAPI*)(Handle,uint32_t*,QueueProperties*);
using CreateDevice=int32_t(WINAPI*)(Handle,const DeviceInfo*,const void*,Handle*);
using DestroyDevice=void(WINAPI*)(Handle,const void*);
using GetQueue=void(WINAPI*)(Handle,uint32_t,uint32_t,Handle*);
template<class T>T load(HMODULE library,const char*name){auto address=GetProcAddress(library,name);if(!address)throw std::runtime_error(name);return reinterpret_cast<T>(address);}
#include "vulkan_geometry_upload_candidate.h"
#define SFM64_TEXTURE_TRANSFER_ONLY
#include "vulkan_buffer_copy_candidate.h"
#include "vulkan_texture_upload_candidate.h"
#include "vulkan_shader_module_candidate.h"
int main(int argc,char**argv){static_assert(sizeof(void*)==8,"x64 required");static_assert(sizeof(InstanceInfo)==64&&sizeof(QueueInfo)==40&&sizeof(DeviceInfo)==72&&sizeof(QueueProperties)==24,"Vulkan ABI");HMODULE library=nullptr;Handle instance=nullptr,device=nullptr;DestroyInstance destroyInstance=nullptr;DestroyDevice destroyDevice=nullptr;try{
 library=LoadLibraryExA("vulkan-1.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);if(!library)throw std::runtime_error("Vulkan loader unavailable");auto createInstance=load<CreateInstance>(library,"vkCreateInstance");destroyInstance=load<DestroyInstance>(library,"vkDestroyInstance");auto enumerate=load<EnumerateDevices>(library,"vkEnumeratePhysicalDevices");auto families=load<QueueFamilies>(library,"vkGetPhysicalDeviceQueueFamilyProperties");auto createDevice=load<CreateDevice>(library,"vkCreateDevice");destroyDevice=load<DestroyDevice>(library,"vkDestroyDevice");auto getQueue=load<GetQueue>(library,"vkGetDeviceQueue");InstanceInfo info{};info.type=1;auto code=createInstance(&info,nullptr,&instance);if(code)throw std::runtime_error("vkCreateInstance result "+std::to_string(code));uint32_t count=0;code=enumerate(instance,&count,nullptr);if(code||!count)throw std::runtime_error("physical device enumeration unavailable");std::vector<Handle>physical(count);code=enumerate(instance,&count,physical.data());if(code)throw std::runtime_error("physical device enumeration changed");physical.resize(count);std::cout<<"physical_devices="<<count<<'\n';bool success=false;
 for(size_t index=0;index<physical.size()&&!success;++index){uint32_t familyCount=0;families(physical[index],&familyCount,nullptr);std::vector<QueueProperties>properties(familyCount);families(physical[index],&familyCount,properties.data());properties.resize(familyCount);for(uint32_t f=0;f<familyCount&&!success;++f){if(!(properties[f].flags&1)||!properties[f].count)continue;float priority=1;QueueInfo queue{};queue.type=2;queue.family=f;queue.count=1;queue.priorities=&priority;DeviceInfo deviceInfo{};deviceInfo.type=3;deviceInfo.queueCount=1;deviceInfo.queues=&queue;code=createDevice(physical[index],&deviceInfo,nullptr,&device);if(code){std::cout<<"device="<<index<<" family="<<f<<" create_result="<<code<<'\n';continue;}Handle graphicsQueue=nullptr;getQueue(device,f,0,&graphicsQueue);if(!graphicsQueue)throw std::runtime_error("null graphics queue");std::cout<<"graphics_device_created=1 device_index="<<index<<" queue_family="<<f<<" family_queue_count="<<properties[f].count<<" queue_flags="<<properties[f].flags<<'\n';if(argc>=2)testVulkanGeometryUploadCandidate(library,physical[index],device,graphicsQueue,f,argv[1]);if(argc>=3)testVulkanTextureUploadCandidate(library,physical[index],device,graphicsQueue,f,argv[2],argv[1],argc>=4?uint32_t(std::stoul(argv[3])):0);success=true;}}
 if(device){destroyDevice(device,nullptr);device=nullptr;}destroyInstance(instance,nullptr);instance=nullptr;FreeLibrary(library);library=nullptr;if(!success)throw std::runtime_error("no graphics device created");std::cout<<"x64 Vulkan graphics availability probe passed\n";return 0;
 }catch(const std::exception&e){if(device&&destroyDevice)destroyDevice(device,nullptr);if(instance&&destroyInstance)destroyInstance(instance,nullptr);if(library)FreeLibrary(library);std::cerr<<e.what()<<'\n';return 1;}}

