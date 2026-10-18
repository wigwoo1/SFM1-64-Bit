from pathlib import Path
import re
p=Path('work/vulkan_swapchain_present_probe_candidate.h');s=p.read_text()
dispatch=s[s.index(' auto getQueue='):s.index('\n VkQueue queue=')]
fields=re.findall(r'auto (\w+)=reinterpret_cast<(PFN_\w+)>',dispatch)
assignments=dispatch
for name,kind in fields:assignments=assignments.replace('auto '+name+'=',name+'=')
create=s[s.index(' VkCommandPoolCreateInfo poolInfo{};'):s.index('\n for(unsigned frame=')]
create=create.replace('VkCommandBuffer command=VK_NULL_HANDLE;','command=VK_NULL_HANDLE;')
body=s[s.index('{check(resetFence',s.index(' for(unsigned frame='))+1:s.index('\n std::cout<<"swapchain presentation probe passed')]
assert body.endswith('}')
body=body[:-1].replace('color.float32[frame]=1','color.float32[frame%3]=1')
cleanup=s[s.index('auto cleanup=[&]{')+len('auto cleanup=[&]{'):s.index('};try{')]
memberDeclarations='\n'.join(' '+kind+' '+name+'=nullptr;' for name,kind in fields)
result='''#pragma once
class RetainedSwapchainPresenterCandidate {
 VkDevice device;VkSwapchainKHR swapchain;std::vector<VkImage>images;VkExtent2D extent;
 VkQueue queue=VK_NULL_HANDLE;VkCommandPool pool=VK_NULL_HANDLE;VkFence acquired=VK_NULL_HANDLE;VkSemaphore rendered=VK_NULL_HANDLE;VkCommandBuffer command=VK_NULL_HANDLE;bool pending=false;size_t frames=0;
'''+memberDeclarations+'''
 static void check(VkResult r){if(r!=VK_SUCCESS&&r!=VK_SUBOPTIMAL_KHR)throw std::runtime_error("presentation result "+std::to_string(r));}
 void cleanup(){'''+cleanup+'''}
public:
 RetainedSwapchainPresenterCandidate(HMODULE library,VkDevice deviceValue,uint32_t family,VkSwapchainKHR chain,const std::vector<VkImage>&handles,VkExtent2D size):device(deviceValue),swapchain(chain),images(handles),extent(size){auto get=entry<PFN_vkGetDeviceProcAddr>(library,"vkGetDeviceProcAddr");
'''+assignments+'''
 getQueue(device,family,0,&queue);if(!queue)throw std::runtime_error("presentation queue missing");try{
'''+create+'''
 }catch(...){cleanup();throw;}}
 ~RetainedSwapchainPresenterCandidate(){cleanup();}
 RetainedSwapchainPresenterCandidate(const RetainedSwapchainPresenterCandidate&)=delete;
 RetainedSwapchainPresenterCandidate&operator=(const RetainedSwapchainPresenterCandidate&)=delete;
 size_t presentedFrames()const{return frames;}
 void presentImage(VkImage source=VK_NULL_HANDLE){if(source&&(!extent.width||!extent.height))throw std::runtime_error("presentation extent missing");auto frame=frames;
'''+body+'''
 ++frames;std::cout<<"swapchain presentation probe passed source="<<(source?"model_gpu_image":"clear")<<" frames=1 sync=serialized visual_pixels=unverified\\n";
 }
};
inline void probeSwapchainPresentCandidate(HMODULE library,VkDevice device,uint32_t family,VkSwapchainKHR swapchain,const std::vector<VkImage>&images,VkImage source=VK_NULL_HANDLE,VkExtent2D extent={}){
 RetainedSwapchainPresenterCandidate presenter(library,device,family,swapchain,images,extent);for(unsigned n=0;n<(source?1u:3u);++n)presenter.presentImage(source);
}
'''
p.write_text(result)
p=Path('work/vulkan_model_swapchain_probe_candidate.h');s=p.read_text().replace('try{size_t presented=0;', 'try{RetainedSwapchainPresenterCandidate presenter(library,device,family,swapchain,images,extent);size_t presented=0;')
s=s.replace('probeSwapchainPresentCandidate(library,device,family,swapchain,images,image,extent);','presenter.presentImage(image);')
s=s.replace('transfer=gpu_image_blit gdi=0', 'transfer=gpu_image_blit gdi=0 presentation_resource_sets=1')
p.write_text(s)
