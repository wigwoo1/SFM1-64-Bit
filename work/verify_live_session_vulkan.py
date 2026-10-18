from pathlib import Path
import json,re
log=Path('outputs/sfm64-live-session-vulkan-validation.txt').read_text()
states=re.findall(r'live_session_frame=(\d+) tick=(\d+) shot=(\d+) rebuilt=(\d+) executed=(\d+)',log)
assert states==[('0','870833','2470','1','499'),('1','880833','2470','0','499'),('2','870833','2470','0','499')],states
assert 'model_gpu_presentation_passed=1 frames=3' in log
refs=('mamagen-vulkan-sampled-shot-v1.rgba','mamagen-vulkan-sampled-shot-second-tick-v1.rgba','mamagen-vulkan-sampled-shot-v1.rgba')
for frame,name in enumerate(refs):assert Path(f'outputs/vulkan-retained-frame-{frame}.rgba').read_bytes()==Path('outputs',name).read_bytes(),frame
Path('outputs/sfm64-live-session-vulkan-summary.json').write_text(json.dumps({'session':'roundtrip-11.dmx','root_clip':2237,'ticks':[870833,880833,870833],'shot':2470,'executed_operations_per_frame':499,'prepared_rebuilds':1,'source_pixels_equal_to_export_path':True,'intermediate_pose_camera_files':'not used','native_scheduler':'unfinished','swapchain_visual_pixels':'unverified'},indent=2)+'\n')
print('live DMX evaluation to Vulkan: 3 frames source-pixel exact with export path, retained bindings and backward seek verified')
for name in ('win32_vulkan_playback_candidate.h','probe_vulkan_win32_surface_x64.cpp','vulkan_model_swapchain_probe_candidate.h'):
 p=Path('work')/name;s=p.read_text().replace('playback.','vulkanPlaybackState.').replace('inline playback;','inline vulkanPlaybackState;');p.write_text(s)
p=Path('work/run_live_session_vulkan_probe.cmd');text=p.read_text();Path('work/run_live_session_control_test.cmd').write_text(text.replace('probe_vulkan_win32_surface_x64.exe\n','probe_vulkan_win32_surface_x64.exe --control-test\n'))
Path('work/open_live_session_vulkan.cmd').write_text(text.replace('setlocal','setlocal\ncd /d "%~dp0.."',1).replace('probe_vulkan_win32_surface_x64.exe\n','probe_vulkan_win32_surface_x64.exe --interactive\n'))
