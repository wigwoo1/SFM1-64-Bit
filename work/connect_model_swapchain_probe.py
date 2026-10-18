from pathlib import Path
p=Path('work/probe_vulkan_win32_surface_x64.cpp');s=p.read_text().replace('#include "vulkan_win32_swapchain_probe_candidate.h"','using Handle=void*;\ntemplate<class T>T load(HMODULE library,const char*name){return entry<T>(library,name);}\n#include "vulkan_win32_swapchain_probe_candidate.h"');p.write_text(s)
p=Path('work/vulkan_win32_swapchain_probe_candidate.h');s=p.read_text().replace('#include "vulkan_swapchain_present_probe_candidate.h"','#include "vulkan_swapchain_present_probe_candidate.h"\n#include "vulkan_model_swapchain_probe_candidate.h"')
s=s.replace('swapchain,handles);cleanup();','swapchain,handles);probeModelSwapchainCandidate(library,physical,device,family,swapchain,handles,format.format,extent);cleanup();');p.write_text(s)
