#include <windows.h>
#include <vulkan/vulkan.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <stdexcept>
template<class T>T get(HMODULE module,const char*name){auto p=GetProcAddress(module,name);if(!p)throw std::runtime_error(name);return reinterpret_cast<T>(p);}
int main(){HMODULE module=LoadLibraryW(L"vulkan-1.dll");if(!module)return 1;VkInstance instance{};
 auto cleanup=[&]{if(instance)get<PFN_vkDestroyInstance>(module,"vkDestroyInstance")(instance,nullptr);FreeLibrary(module);};
 auto check=[](VkResult r){if(r!=VK_SUCCESS)throw std::runtime_error("Vulkan result "+std::to_string(r));};
 try{VkApplicationInfo app{};app.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO;app.pApplicationName="SFM x64 bias capability audit";app.apiVersion=VK_API_VERSION_1_1;VkInstanceCreateInfo create{};create.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;create.pApplicationInfo=&app;check(get<PFN_vkCreateInstance>(module,"vkCreateInstance")(&create,nullptr,&instance));
 uint32_t count=0;auto enumerate=get<PFN_vkEnumeratePhysicalDevices>(module,"vkEnumeratePhysicalDevices");check(enumerate(instance,&count,nullptr));if(!count)throw std::runtime_error("no physical devices");std::vector<VkPhysicalDevice>devices(count);check(enumerate(instance,&count,devices.data()));
 for(uint32_t n=0;n<count;++n){VkPhysicalDeviceProperties props{};get<PFN_vkGetPhysicalDeviceProperties>(module,"vkGetPhysicalDeviceProperties")(devices[n],&props);uint32_t extCount=0;auto extensions=get<PFN_vkEnumerateDeviceExtensionProperties>(module,"vkEnumerateDeviceExtensionProperties");check(extensions(devices[n],nullptr,&extCount,nullptr));std::vector<VkExtensionProperties>available(extCount);check(extensions(devices[n],nullptr,&extCount,available.data()));bool supported=false;for(auto&e:available)if(!std::strcmp(e.extensionName,VK_EXT_DEPTH_BIAS_CONTROL_EXTENSION_NAME))supported=true;
 VkPhysicalDeviceDepthBiasControlFeaturesEXT bias{};bias.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_BIAS_CONTROL_FEATURES_EXT;VkPhysicalDeviceFeatures2 features{};features.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;features.pNext=supported?&bias:nullptr;get<PFN_vkGetPhysicalDeviceFeatures2>(module,"vkGetPhysicalDeviceFeatures2")(devices[n],&features);
 std::cout<<"device="<<n<<" name="<<props.deviceName<<" vendor="<<props.vendorID<<" device_id="<<props.deviceID<<" api="<<props.apiVersion<<" extension="<<supported<<" depth_bias_control="<<bias.depthBiasControl<<" float_representation="<<bias.floatRepresentation<<" exact="<<bias.depthBiasExact<<" force_unorm="<<bias.leastRepresentableValueForceUnormRepresentation<<"\n";
 }cleanup();return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";cleanup();return 1;}}
