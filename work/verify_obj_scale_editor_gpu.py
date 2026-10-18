from pathlib import Path
import runpy,subprocess,json
state=runpy.run_path('work/verify_obj_gpu_import.py');out=state['out'];folder=out/'scale-editor';folder.mkdir(exist_ok=True)
env=state['env'].copy();env.update(SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(folder.resolve()),SFM64_DIAGNOSTIC_EDITOR_CONTROL_TEST='1',SFM64_DIAGNOSTIC_OBJ_SCALE_CONTROL_TEST='1')
run=subprocess.run(state['args'],env=env,capture_output=True,text=True,timeout=60);log=run.stdout+run.stderr;(out/'scale-editor.txt').write_text(log);assert run.returncode==0,log
factors=[line.split(' factor=')[1].split()[0] for line in log.splitlines() if 'obj_scale_editor_action_applied factor=' in line];assert factors==['0.5','1','0.5','1'],factors
frames=[(folder/f'vulkan-retained-submission-{i}.rgba').read_bytes() for i in range(6)]
assert frames[1]==frames[3]==frames[5],'double/reset did not restore rendered geometry'
assert frames[2]==frames[4] and frames[2]!=frames[1],'half scale did not refresh rendered geometry'
white=lambda frame:sum(frame[i:i+3]==b'\xff\xff\xff' for i in range(0,len(frame),4))
full=white(frames[1]);half=white(frames[2]);assert full>90000 and 20000<half<full*.5,(full,half)
baseline=(out/'baseline'/'vulkan-retained-frame-1.rgba').read_bytes();outside=0;inner=0;changed=0
for y in range(512):
 for x in range(512):
  i=(y*512+x)*4
  if frames[2][i:i+4]!=baseline[i:i+4]:
   changed+=1
   if not (178<=x<=334 and 178<=y<=334):outside+=1
  if 181<=x<=330 and 181<=y<=330 and frames[2][i:i+3]==b'\xff\xff\xff':inner+=1
assert outside==0 and inner==22500 and 10000<changed<35000,(outside,inner,changed)
report=dict(actual_gpu=True,real_win32_menu_commands=True,applied_factors=factors,full_white_pixels=full,half_white_pixels=half,half_import_changed_pixels=changed,outside_expected_bounds=outside,inner_white_pixels=inner,double_and_reset_restore_exact_pixels=True,persistence='unfinished')
(out/'scale-editor-report.json').write_text(json.dumps(report,indent=2)+'\n');print('obj_scale_editor_gpu=passed '+str(report))
