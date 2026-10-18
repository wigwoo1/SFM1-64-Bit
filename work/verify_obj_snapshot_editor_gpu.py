import os,runpy,subprocess,json,shutil
state=runpy.run_path('work/verify_obj_png_gpu.py');base=state['state'];out=state['out'];folder=out/'snapshot-editor';folder.mkdir(exist_ok=True);inputs=folder/'inputs';inputs.mkdir(exist_ok=True)
paths=[]
for source in [base['obj'],state['red'],state['blue']]:
 target=inputs/source.name;shutil.copyfile(source,target);paths.append(target)
args=base['args'][:-2]+['--obj',str(paths[0]),'--obj-scale','0.5','--obj-texture','left',str(paths[1]),'--obj-texture','right',str(paths[2])]
snapshot=folder/'model.sfmobj';env=base['env'].copy();env.update(SFM64_DIAGNOSTIC_EDITOR_CONTROL_TEST='1',SFM64_DIAGNOSTIC_OBJ_SNAPSHOT_TEST='1',SFM64_DIAGNOSTIC_OBJ_SNAPSHOT_PATH=str(snapshot.resolve()))
def run(args,env,output):
 output.mkdir(exist_ok=True);env=env.copy();env['SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY']=str(output.resolve());result=subprocess.run(args,env=env,capture_output=True,text=True,timeout=int(os.environ.get('SFM64_TEST_TIMEOUT_SECONDS','240')));log=result.stdout+result.stderr;(output/'log.txt').write_text(log);assert result.returncode==0,log;return log
log=run(args,env,folder/'saved');assert 'obj_snapshot_saved=1' in log and 'obj_snapshot_loaded=1' in log and 'editor_action_rejected=' not in log
frames=[(folder/'saved'/f'vulkan-retained-submission-{i}.rgba').read_bytes() for i in range(5)];assert frames[1]==frames[2]==frames[4] and frames[3]!=frames[4]
for path in paths:path.unlink()
env.pop('SFM64_DIAGNOSTIC_OBJ_SNAPSHOT_TEST');env['SFM64_DIAGNOSTIC_OBJ_SNAPSHOT_LOAD_TEST']='1';log=run(base['args'][:-2],env,folder/'reopened');assert 'obj_snapshot_loaded=1' in log and 'editor_action_rejected=' not in log
assert (folder/'reopened'/'vulkan-retained-submission-2.rgba').read_bytes()==frames[1]
valid=snapshot.read_bytes();invalid={ 'truncated':valid[:-1], 'version':valid[:8]+(2).to_bytes(4,'little')+valid[12:], 'trailing':valid+b'x','nonfinite_scale':valid[:12]+bytes.fromhex('0000807f')+valid[16:]}
for name,data in invalid.items():
 path=folder/(name+'.sfmobj');path.write_bytes(data);env['SFM64_DIAGNOSTIC_OBJ_SNAPSHOT_PATH']=str(path.resolve());log=run(base['args'],env,folder/name);assert 'editor_action_rejected=' in log and 'obj_snapshot_loaded=1' not in log
 before=(folder/name/'vulkan-retained-submission-1.rgba').read_bytes();after=(folder/name/'vulkan-retained-submission-2.rgba').read_bytes();assert before==after
report=dict(actual_gpu=True,real_win32_commands=True,reopen_new_process_exact_pixels=True,source_obj_and_png_removed=True,scale_and_slot_textures_preserved=True,rejected=list(invalid),bytes=len(valid),limits='Separate imported-model snapshot; not yet integrated with DMX session Save As. One OBJ owner, opaque textures; FBX and full project persistence unfinished.')
(folder/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('obj_snapshot_gpu=passed '+str(report))
