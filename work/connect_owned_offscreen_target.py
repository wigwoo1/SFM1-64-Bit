from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text();s='#include "vulkan_offscreen_target_candidate.h"\n'+s
# The existing first include defines Vulkan types before this target helper.
s=s.replace('#include "vulkan_offscreen_target_candidate.h"\n','',1)
lines=s.splitlines();lines.insert(1,'#include "vulkan_offscreen_target_candidate.h"');s='\n'.join(lines)+'\n'
old='VkImage image=VK_NULL_HANDLE,depthImage=VK_NULL_HANDLE;'
assert old in s;s=s.replace(old,'std::unique_ptr<VulkanOffscreenTargetCandidate>offscreenTarget;VkImage image=VK_NULL_HANDLE,depthImage=VK_NULL_HANDLE;',1)
old='if(framebuffer)destroyFramebuffer(dev,framebuffer,nullptr);if(depthView)destroyView(dev,depthView,nullptr);if(view)destroyView(dev,view,nullptr);if(depthImage)destroyImage(dev,depthImage,nullptr);if(image)destroyImage(dev,image,nullptr);'
assert old in s;s=s.replace(old,'offscreenTarget.reset();',1)
start=s.index('  VkImageCreateInfo imageInfo{};')
end=s.index('VkCommandPoolCreateInfo poolInfo{};',start)
s=s[:start]+'  offscreenTarget=std::make_unique<VulkanOffscreenTargetCandidate>(library,dev,gpu,pass,outputWidth,outputHeight);image=offscreenTarget->color;depthImage=offscreenTarget->depth;view=offscreenTarget->colorView;depthView=offscreenTarget->depthView;framebuffer=offscreenTarget->framebuffer;'+s[end:]
p.write_text(s)
