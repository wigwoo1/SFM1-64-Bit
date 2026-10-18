from pathlib import Path
p=Path('work/vulkan_swapchain_present_probe_candidate.h');s=p.read_text()
s=s.replace('#pragma once','#pragma once\n#include "viewport_fit_candidate.h"')
s=s.replace('if(source){VkImageBlit region{};', '''if(source){auto fit=viewportFitCandidate(512,512,extent.width,extent.height);VkClearColorValue background{};background.float32[3]=1;clear(command,images[index],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&background,1,&transition.subresourceRange);VkMemoryBarrier clearToBlit{};clearToBlit.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;clearToBlit.srcAccessMask=clearToBlit.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,1,&clearToBlit,0,nullptr,0,nullptr);VkImageBlit region{};''')
s=s.replace('region.dstOffsets[1]={int32_t(extent.width),int32_t(extent.height),1};', 'region.dstOffsets[0]={fit.x,fit.y,0};region.dstOffsets[1]={fit.x+fit.width,fit.y+fit.height,1};std::cout<<"viewport_fit x="<<fit.x<<" y="<<fit.y<<" width="<<fit.width<<" height="<<fit.height<<" background=black\\n";')
p.write_text(s)
