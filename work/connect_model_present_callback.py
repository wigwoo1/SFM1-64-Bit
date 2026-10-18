from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text().replace('#include <future>','#include <future>\n#include <functional>')
s=s.replace('const std::string&base){','const std::string&base,const std::function<void(VkImage)>&presentImage={}){',1)
s=s.replace('++submissionFenceWaits;pending=false;','++submissionFenceWaits;pending=false;if(presentImage)presentImage(image);')
p.write_text(s)
p=Path('work/vulkan_graphics_pipeline_candidate.h');s=p.read_text().replace('const std::string&base){','const std::string&base,const std::function<void(VkImage)>&presentImage={}){',1)
s=s.replace('layout,setLayout,base);','layout,setLayout,base,presentImage);');p.write_text(s)
p=Path('work/vulkan_swapchain_present_probe_candidate.h');s=p.read_text().replace('const std::vector<VkImage>&images){','const std::vector<VkImage>&images,VkImage source=VK_NULL_HANDLE,VkExtent2D extent={}){')
s=s.replace('auto clear=reinterpret_cast', 'auto blit=reinterpret_cast<PFN_vkCmdBlitImage>(get(device,"vkCmdBlitImage"));auto clear=reinterpret_cast')
s=s.replace('if(!getQueue||','if(!blit||!getQueue||')
s=s.replace('frame<3;', 'frame<(source?1u:3u);')
old='clear(command,images[index],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&color,1,&transition.subresourceRange);'
assert old in s
s=s.replace(old,'if(source){VkImageBlit region{};region.srcSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};region.dstSubresource=region.srcSubresource;region.srcOffsets[1]={512,512,1};region.dstOffsets[1]={int32_t(extent.width),int32_t(extent.height),1};blit(command,source,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,images[index],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&region,VK_FILTER_NEAREST);}else '+old)
s=s.replace('presented_clear_frame=', 'presented_frame=')
s=s.replace('swapchain presentation probe passed frames=3 sync=serialized visual_pixels=unverified','swapchain presentation probe passed source="<<(source?"model_gpu_image":"clear")<<" frames="<<(source?1:3)<<" sync=serialized visual_pixels=unverified')
p.write_text(s)
