import runpy,subprocess,json
state=runpy.run_path('work/verify_obj_png_gpu.py');out=state['out'];base=state['state'];reports={}
def run(args,env,folder):
 folder.mkdir(exist_ok=True);env=env.copy();env['SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY']=str(folder.resolve());result=subprocess.run(args,env=env,capture_output=True,text=True,timeout=60);log=result.stdout+result.stderr;(folder/'log.txt').write_text(log);assert result.returncode==0,log;return log
refs={}
for key,assignments in [('left-red',[('left',state['red'])]),('both',[('left',state['red']),('right',state['blue'])]),('both-blue',[('left',state['blue']),('right',state['blue'])])]:
 folder=out/('texture-ref-'+key);args=base['args']+['--obj-scale','0.5']
 for name,path in assignments:args+=['--obj-texture',name,str(path)]
 run(args,base['env'],folder);refs[key]=(folder/'vulkan-retained-frame-1.rgba').read_bytes()
for mode in ['assign-replace','corrupt','transparent','unknown','cancel']:
 folder=out/('texture-editor-'+mode);env=base['env'].copy();env.update(SFM64_DIAGNOSTIC_EDITOR_CONTROL_TEST='1',SFM64_DIAGNOSTIC_OBJ_TEXTURE_CONTROL_TEST='1')
 names=['left','right','left'];paths=[state['red'],state['blue'],state['blue']]
 if mode=='corrupt':paths[0]=state['broken']
 if mode=='transparent':paths[0]=state['transparent']
 if mode=='unknown':names[0]='missing'
 for i,(name,path) in enumerate(zip(names,paths),1):
  env['SFM64_DIAGNOSTIC_OBJ_TEXTURE_MATERIAL_'+str(i)]=name
  if not(mode=='cancel' and i==1):env['SFM64_DIAGNOSTIC_OBJ_TEXTURE_PATH_'+str(i)]=str(path.resolve())
 log=run(base['args']+['--obj-scale','0.5'],env,folder);frames=[(folder/f'vulkan-retained-submission-{i}.rgba').read_bytes() for i in range(5)]
 if mode=='assign-replace':assert frames[2]==refs['left-red'] and frames[3]==refs['both'] and frames[4]==refs['both-blue'];assert log.count('obj_texture_editor_action_applied')==3
 else:
  assert frames[1]==frames[2],'invalid/cancel assignment changed pixels: '+mode
  assert frames[4]==refs['both-blue'],'valid later replacement failed: '+mode
  assert log.count('obj_texture_editor_action_applied')==2
  if mode=='cancel':assert 'obj_texture_editor_cancelled=1' in log
  else:assert 'editor_action_rejected=' in log
 reports[mode]=dict(exact_gpu_pixels=True,scale_preserved=True,retained_previous_texture=mode!='assign-replace')
report=dict(real_win32_menu_commands=True,actual_gpu=True,cases=reports,limits='Opaque PNGs only. Visible material popup/file picker interaction, texture settings, undo, persistence and FBX unfinished.')
(out/'texture-editor-report.json').write_text(json.dumps(report,indent=2)+'\n');print('obj_texture_editor_gpu=passed '+str(report))
