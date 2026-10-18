import os
import runpy,subprocess,json
state=runpy.run_path('work/verify_obj_gpu_import.py');out=state['out'];reports={}
for mode in ['scale','import','texture']:
 folder=out/('history-'+mode);folder.mkdir(exist_ok=True);env=state['env'].copy();env.update(SFM64_DIAGNOSTIC_EDITOR_CONTROL_TEST='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(folder.resolve()));args=state['args']
 if mode=='scale':env['SFM64_DIAGNOSTIC_OBJ_HISTORY_TEST']='1';count=8
 elif mode=='import':env.update(SFM64_DIAGNOSTIC_OBJ_IMPORT_HISTORY_TEST='1',SFM64_DIAGNOSTIC_OBJ_IMPORT_PATH=str(state['obj'].resolve()));args=args[:-2];count=5
 else:
  env['SFM64_DIAGNOSTIC_OBJ_TEXTURE_HISTORY_TEST']='1';count=8
  for step,name,path in [(1,'left',out/'red.png'),(4,'right',out/'blue.png')]:env['SFM64_DIAGNOSTIC_OBJ_TEXTURE_MATERIAL_'+str(step)]=name;env['SFM64_DIAGNOSTIC_OBJ_TEXTURE_PATH_'+str(step)]=str(path.resolve())
 result=subprocess.run(args,env=env,capture_output=True,text=True,timeout=int(os.environ.get("SFM64_TEST_TIMEOUT_SECONDS","60")));log=result.stdout+result.stderr;(folder/'log.txt').write_text(log);assert result.returncode==0 and 'editor_action_rejected=' not in log,log
 frames=[(folder/f'vulkan-retained-submission-{i}.rgba').read_bytes() for i in range(count)]
 if mode=='scale':
  assert frames[1]==frames[3]==frames[5] and frames[2]==frames[4] and frames[1]!=frames[2];assert frames[6]==frames[7] and frames[6]!=frames[1];assert 'changed=0 undo=1 redo=0' in log
 elif mode=='import':assert frames[1]==frames[3] and frames[2]==frames[4] and frames[1]!=frames[2]
 else:assert frames[1]==frames[3] and frames[2]==frames[4]==frames[6] and frames[5]==frames[7] and frames[5]!=frames[2]
 reports[mode]=dict(exact_gpu_undo_redo=True,branch_discards_redo=mode=='scale')
(out/'history-editor-report.json').write_text(json.dumps(reports,indent=2)+'\n');print('obj_editor_history_gpu=passed '+str(reports))
