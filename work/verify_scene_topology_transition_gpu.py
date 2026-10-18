from pathlib import Path
import subprocess,os,json,hashlib,re,argparse
parser=argparse.ArgumentParser();parser.add_argument('--static-flex',action='store_true');args=parser.parse_args()
prefix='flex-scene-topology-transition' if args.static_flex else 'scene-topology-transition'
records=[];baseline=None;four=None
for name,mode,ticks in [('sync',0,[870833,1259500,870833]),('prefetch',1,[870833,1259500,870833]),('four-independent',0,[1259500,1259500,1259500])]:
 path=Path(f'work/{prefix}-{name}-ticks.txt');path.write_text('\n'.join(map(str,ticks))+'\n')
 directory=Path(f'outputs/{prefix}-{name}');directory.mkdir(exist_ok=True)
 env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
 env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_PRESENT_MODEL=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\anno\mamagen\mamagen',SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK=str(ticks[0]),SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION='work/roundtrip-11.dmx',SFM64_DIAGNOSTIC_SCENE_TICKS=str(path),SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_FRAME_PREFETCH=str(mode),SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
 env['SFM64_DIAGNOSTIC_STATIC_FLEX']='1' if args.static_flex else '0'
 run=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'],env=env,capture_output=True,text=True,timeout=60)
 log=run.stdout+run.stderr;Path(f'outputs/sfm64-{prefix}-{name}.txt').write_text(log)
 assert run.returncode==0,log
 models=[int(n) for n in re.findall(r'multi_model_scene_frame=\d+ tick=\d+ models=(\d+)',log)]
 pixels=[(directory/f'vulkan-retained-frame-{n}.rgba').read_bytes() for n in range(3)]
 assert all(len(p)==512*512*4 for p in pixels)
 independent=name=='four-independent'
 assert models==([4,4,4] if independent else [1,4,1])
 assert f'geometry_allocations={1 if independent else 2} material_uploads={1 if independent else 2}' in log
 assert f'retained_gpu_material_cache_entries={4 if independent else 5}' in log
 if independent:assert all(p==four for p in pixels),'transitioned four-model frame differs from independent shot'
 else:
  assert pixels[0]==pixels[2] and pixels[0]!=pixels[1]
  if args.static_flex:
   for index,scene in ((0,'static-flex'),(1,'four-model-static-flex')):
    reference=json.loads(Path(f'outputs/sfm64-{scene}-before-borrowed-output.json').read_text(encoding='utf-8'))
    expected=next(r for r in reference['runs'] if r['enabled']==1 and r['workers']==4 and r['prefetch']==0)['frame_sha256'][0]
    assert hashlib.sha256(pixels[index]).hexdigest()==expected,'transition differs from pre-reuse independent scene'
  else:assert pixels[0]==Path('outputs/material-overrides-prefetch-0/vulkan-retained-frame-0.rgba').read_bytes(),'initial/restored single-model frame changed'
  if baseline is None:baseline=pixels;four=pixels[1]
  else:assert pixels==baseline,'prefetch changed topology transition pixels'
 records.append(dict(name=name,models=models,frame_sha256=[hashlib.sha256(p).hexdigest() for p in pixels],geometry_allocations=1 if independent else 2,gpu_material_cache_entries=4 if independent else 5))
Path(f'outputs/sfm64-{prefix}-gpu.json').write_text(json.dumps(dict(static_flex=args.static_flex,real_saved_shots=True,models_sequence=[1,4,1],restore_pixel_exact=True,four_model_frame_matches_independent_shot=True,prefetch_pixel_exact=True,runs=records,scope='GPU0 two real saved shots, grown host-visible geometry buffers and cached materials; not full session/editor/native renderer compatibility'),indent=2)+'\n')
print('real saved shot topology transition passed: 1 -> 4 -> 1 models, independent-shot pixels exact, buffer growth reused on return, prefetch exact')
