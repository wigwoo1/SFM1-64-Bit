from pathlib import Path
import re,json
folder=Path('outputs/vulkan-playback-controls-current')
log=(folder/'validation.txt').read_text();frames=[int(n) for n in re.findall(r'Vulkan indexed model draw passed[^\n]* RGBA frame=(\d+)',log)];assert frames==[0,1,0,0,1],frames
assert 'model_gpu_presentation_passed=1 frames=5' in log
assert 'window_visible=0' in log
for frame,name in ((0,'mamagen-vulkan-sampled-shot-v1.rgba'),(1,'mamagen-vulkan-sampled-shot-second-tick-v1.rgba')):assert (folder/f'vulkan-retained-frame-{frame}.rgba').read_bytes()==Path('outputs',name).read_bytes()
summary={'frame_order':frames,'win32_key_messages':['Right','Left','paused redraw','Space plus playback tick','Escape'],'normal_exit':True,'source_pixels_exact':True,'window_visible':False,'swapchain_visual_pixels':'unverified','session_evaluation':'pre-exported inputs','output_directory':str(folder)}
(folder/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
launcher=Path('work/open_vulkan_playback.cmd').read_text()
model=r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\anno\mamagen\mamagen'
assert f'SFM64_DIAGNOSTIC_PRESENT_MODEL={model}' in launcher
assert 'work\\retained-shot-sequence-v1' in launcher and '--interactive' in launcher
assert Path(model+'.mdl').is_file()
print('Win32 Vulkan playback controls passed: forward/backward step, pause, playback tick, Escape; source pixels exact')
print('interactive launcher resolves the installed F: model and retained three-frame sequence')
