import runpy,subprocess,json
state=runpy.run_path('work/verify_obj_gpu_import.py');out=state['out'];reports={}
invalid=out/'invalid-editor.obj';invalid.write_text('v 0 0 0\nf 1 2 3\n')
empty=out/'empty-editor.obj';empty.write_text('v 0 0 0\n')
for mode in ['import','invalid','empty','cancel']:
 folder=out/('editor-import-'+mode);folder.mkdir(exist_ok=True);env=state['env'].copy();env.update(SFM64_DIAGNOSTIC_EDITOR_CONTROL_TEST='1',SFM64_DIAGNOSTIC_OBJ_IMPORT_CONTROL_TEST='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(folder.resolve()))
 args=state['args'][:-2] if mode=='import' else state['args']
 if mode!='cancel':env['SFM64_DIAGNOSTIC_OBJ_IMPORT_PATH']=str((state['obj'] if mode=='import' else invalid if mode=='invalid' else empty).resolve())
 run=subprocess.run(args,env=env,capture_output=True,text=True,timeout=60);log=run.stdout+run.stderr;(out/('editor-import-'+mode+'.txt')).write_text(log);assert run.returncode==0,log
 frames=[(folder/f'vulkan-retained-submission-{i}.rgba').read_bytes() for i in range(5)]
 expected=(out/'imported'/'vulkan-retained-frame-1.rgba').read_bytes()
 assert frames[2]==frames[4]==expected,'import/reset pixels mismatch: '+mode
 assert frames[3]!=frames[2],'scale after import did not refresh: '+mode
 if mode=='import':
  baseline=(out/'baseline'/'vulkan-retained-frame-1.rgba').read_bytes();assert frames[1]==baseline and frames[2]!=baseline
  assert 'obj_import_editor_action_applied vertices=6' in log and 'editor_action_rejected=' not in log
 else:
  assert frames[1]==frames[2],'failed/cancelled import replaced existing geometry'
  if mode=='cancel':assert 'obj_import_editor_cancelled=1' in log
  else:assert 'editor_action_rejected=' in log and 'obj_import_editor_action_applied' not in log
 reports[mode]=dict(exact_gpu_pixels_verified=True,existing_owner_preserved=mode!='import')
report=dict(actual_gpu=True,real_win32_menu_commands=True,cases=reports,limits='One imported OBJ owner. File dialog path supplied diagnostically; visible picker interaction unverified. PNG picker, FBX, multiple imports, undo and save persistence unfinished.')
(out/'editor-import-report.json').write_text(json.dumps(report,indent=2)+'\n');print('obj_import_editor_gpu=passed '+str(report))
