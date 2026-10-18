from pathlib import Path
p=Path('work/probe_vulkan_win32_surface_x64.cpp');s=p.read_text()
s=s.replace('int main(){','#include "vulkan_win32_swapchain_probe_candidate.h"\nint main(){')
old='for(auto mode:presentModes)std::cout<<"present_mode="<<mode<<\'\\n\';'
assert old in s
s=s.replace(old,old+'probeWin32SwapchainCandidate(library,devices[n],f,surface,caps,surfaceFormats);')
s=s.replace('swapchain=unfinished','swapchain=created presentation=unfinished')
p.write_text(s)
