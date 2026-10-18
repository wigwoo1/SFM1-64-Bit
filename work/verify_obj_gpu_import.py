from pathlib import Path
import subprocess,os,struct,json,uuid
out=Path('outputs/obj-gpu-import');out.mkdir(exist_ok=True)
matrix=out/('camera-'+str(uuid.uuid4())+'.bin')
export=subprocess.run(['build/session-core/export_session_camera_candidate_x64.exe','work/short-timeline-session.dmx',str(matrix),'engine'],capture_output=True,text=True,timeout=30);assert export.returncode==0,export.stdout+export.stderr
values=struct.unpack('<16f',matrix.read_bytes());a=[[float(values[4*r+c]) for c in range(4)]+[float(r==c) for c in range(4)] for r in range(4)]
for c in range(4):
 pivot=max(range(c,4),key=lambda r:abs(a[r][c]));a[c],a[pivot]=a[pivot],a[c];assert abs(a[c][c])>1e-15
 factor=a[c][c];a[c]=[x/factor for x in a[c]]
 for r in range(4):
  if r!=c:
   f=a[r][c];a[r]=[a[r][i]-f*a[c][i] for i in range(8)]
inverse=[r[4:] for r in a];points=[]
for x,y in [(-.6,-.6),(.6,-.6),(.6,.6),(-.6,.6)]:
 clip=[x,y,.3,1];world=[sum(inverse[r][c]*clip[c] for c in range(4)) for r in range(4)];points.append([v/world[3] for v in world[:3]])
obj=out/'camera-facing.obj';obj.write_text('\n'.join('v '+' '.join(format(v,'.9g') for v in p) for p in points)+'\nusemtl left\nf 1 2 3\nusemtl right\nf 1 3 4\n')
logs={};frames={}
for mode in ['baseline','imported']:
 folder=out/mode;folder.mkdir(exist_ok=True);env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')};env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(folder.resolve()))
 args=['build/session-core/probe_vulkan_win32_surface_x64.exe','--session','work/short-timeline-session.dmx','--content-directory',r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod','--control-test']
 if mode=='imported':args+=['--obj',str(obj)]
 run=subprocess.run(args,env=env,capture_output=True,text=True,timeout=int(os.environ.get("SFM64_TEST_TIMEOUT_SECONDS","60")));log=run.stdout+run.stderr;(out/(mode+'.txt')).write_text(log);assert run.returncode==0,log;logs[mode]=log
 frame=folder/'vulkan-retained-frame-0.rgba';assert frame.is_file(),log;frames[mode]=frame.read_bytes();assert len(frames[mode])==512*512*4
assert 'scene_obj_attached=1 vertices=6 materials=2' in logs['imported'],logs['imported']
changed=sum(frames['baseline'][i:i+4]!=frames['imported'][i:i+4] for i in range(0,len(frames['baseline']),4));white=sum(frames['imported'][i:i+3]==b'\xff\xff\xff' for i in range(0,len(frames['imported']),4));assert changed>10000 and white>10000,(changed,white)
report=dict(actual_gpu=True,imported_obj=str(obj),changed_pixels=changed,white_preview_pixels=white,material_slots=2,limits='Native OBJ CLI import with existing session camera and white unlit preview materials. UI controls, MTL/PNG, scaling and persistence unfinished.')
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('obj_native_application_vulkan_import=passed changed_pixels='+str(changed)+' white_preview_pixels='+str(white))
