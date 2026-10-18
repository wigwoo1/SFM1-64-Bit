from pathlib import Path
import os,subprocess,json,hashlib,re
ticks=[1259500+(n*10000)//60 for n in range(61)]+[1259500]
tick_path=Path('work/four-model-prefetch-ticks.txt');tick_path.write_text('\n'.join(map(str,ticks))+'\n')
records=[];baseline=None
for mode in (0,1):
 directory=Path(f'outputs/animated-four-model-prefetch-{mode}');directory.mkdir(exist_ok=True)
 env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
 env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_PRESENT_MODEL=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\anno\mamagen\mamagen',SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK=str(ticks[0]),SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION='work/roundtrip-11.dmx',SFM64_DIAGNOSTIC_SCENE_TICKS=str(tick_path.resolve()),SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_FRAME_PREFETCH=str(mode),SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
 run=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'],env=env,capture_output=True,text=True,timeout=60)
 log=run.stdout+run.stderr;Path(f'outputs/sfm64-scene-prefetch-{mode}.txt').write_text(log)
 assert run.returncode==0,log
 assert f'preparation_tasks={61 if mode else 0}' in log
 assert f'preparation_worker_threads={mode} lifetime=retained queue_capacity=1' in log
 assert 'geometry_allocations=1 material_uploads=1' in log
 assert [(int(a),int(b)) for a,b in re.findall(r'multi_model_scene_frame=(\d+) tick=(\d+)',log)]==list(enumerate(ticks))
 hashes=[]
 for n in range(len(ticks)):
  p=(directory/f'vulkan-retained-frame-{n}.rgba').read_bytes();assert len(p)==512*512*4
  hashes.append(hashlib.sha256(p).hexdigest())
 assert hashes[0]==hashes[-1]
 assert len(set(hashes))>1
 if baseline is None:baseline=hashes
 else:assert hashes==baseline,'prefetch changed saved scene pixels'
 records.append(dict(prefetch=mode,frames=len(ticks),preparation_tasks=61 if mode else 0,frame_sha256=hashes))
Path('outputs/sfm64-scene-prefetch-pixel-equivalence.json').write_text(json.dumps(dict(all_frames_pixel_exact=True,backward_seek_pixel_exact=True,models=4,runs=records,scope='GPU0 fixed resources and topology, one outstanding sequential scene evaluation; no native rendering parity claim'),indent=2)+'\n')
print('62 four-model GPU frames exact between synchronous and overlapped scene preparation; backward seek exact')
