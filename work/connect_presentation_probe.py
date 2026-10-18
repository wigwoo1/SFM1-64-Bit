from pathlib import Path
p=Path('work/vulkan_win32_swapchain_probe_candidate.h');s=p.read_text().replace('#pragma once','#pragma once\n#include "vulkan_swapchain_present_probe_candidate.h"')
old='presentation=not_executed\\n";cleanup();'
assert old in s
s=s.replace(old,'presentation=clear_probe\\n";if(!(info.imageUsage&VK_IMAGE_USAGE_TRANSFER_DST_BIT))throw std::runtime_error("clear presentation requires transfer destination");probeSwapchainPresentCandidate(library,device,family,swapchain,handles);cleanup();')
p.write_text(s)
p=Path('work/probe_vulkan_win32_surface_x64.cpp');s=p.read_text().replace('presentation=unfinished','presentation=clear_frames_verified model_presentation=unfinished');p.write_text(s)
