from pathlib import Path
p=Path('work/probe_vulkan_win32_surface_x64.cpp');s=p.read_text()
old='for(size_t n=0;n<devices.size();++n){uint32_t familyCount='
assert old in s
s=s.replace(old,'char deviceText[8]{};auto deviceSize=GetEnvironmentVariableA("SFM64_DIAGNOSTIC_PRESENT_DEVICE",deviceText,sizeof(deviceText));int selectedDevice=-1;if(deviceSize){if(deviceSize!=1||deviceText[0]<\'0\'||deviceText[0]>\'9\')throw std::runtime_error("present device index");selectedDevice=deviceText[0]-\'0\';}\n for(size_t n=0;n<devices.size();++n){if(selectedDevice>=0&&n!=size_t(selectedDevice))continue;uint32_t familyCount=')
s=s.replace('model_presentation=unfinished','interactive_model_presentation=unfinished')
p.write_text(s)
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text().replace('queue_idle_waits=0 preparation_tasks=', 'queue_idle_waits=0 wait_scope=draw_only preparation_tasks=');p.write_text(s)
p=Path('work/run_model_swapchain_probe.cmd');s=p.read_text().replace('setlocal','setlocal\nif not "%~1"=="" set "SFM64_DIAGNOSTIC_PRESENT_DEVICE=%~1"');p.write_text(s)
