from pathlib import Path
import os,subprocess,json,hashlib,argparse

parser=argparse.ArgumentParser()
parser.add_argument('--four-model-scene',action='store_true')
parser.add_argument('--software',action='store_true')
args=parser.parse_args()
executable=Path('build/session-core/probe_vulkan_win32_surface_x64.exe')
executable_hash=hashlib.sha256(executable.read_bytes()).hexdigest()
start=1259500 if args.four_model_scene else 870833
prefix='four-model-static-flex' if args.four_model_scene else 'static-flex'
if args.software:prefix='software-'+prefix
ticks=[start+n*10000//60 for n in range(61)]+[start]
tick_path=Path(f'work/{prefix}-ticks.txt')
tick_path.write_text('\n'.join(map(str,ticks))+'\n',encoding='utf-8')
records=[]
reference=None
baseline=None
for enabled,workers,prefetch in [(0,4,0),(1,1,0),(1,4,0),(1,4,1),(0,4,1)]:
    directory=Path(f'outputs/{prefix}-{enabled}-workers-{workers}-prefetch-{prefetch}')
    directory.mkdir(exist_ok=True)
    env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
    env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_PRESENT_MODEL=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\anno\mamagen\mamagen',SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK=str(ticks[0]),SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION='work/roundtrip-11.dmx',SFM64_DIAGNOSTIC_SCENE_TICKS=str(tick_path.resolve()),SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_FRAME_PREFETCH=str(prefetch),SFM64_DIAGNOSTIC_GEOMETRY_WORKERS=str(workers),SFM64_DIAGNOSTIC_STATIC_FLEX=str(enabled),SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
    env['SFM64_DIAGNOSTIC_SOFTWARE_FLEX']='1' if args.software and enabled else '0'
    run=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'],env=env,capture_output=True,text=True,timeout=60)
    log=run.stdout+run.stderr
    (directory/'run.log').write_text(log,encoding='utf-8')
    assert run.returncode==0,log
    assert ('scene_static_flex=1' in log)==bool(enabled),log
    images=[(directory/f'vulkan-retained-frame-{n}.rgba').read_bytes() for n in range(len(ticks))]
    assert all(len(x)==512*512*4 for x in images)
    assert images[0]==images[-1],'backward seek changed output'
    assert len(set(images))>1,'animation did not change output'
    hashes=[hashlib.sha256(x).hexdigest() for x in images]
    if enabled:
        if reference is None:reference=hashes
        else:assert hashes==reference,'worker or prefetch output differs'
        changed=sum(images[0][i:i+4]!=baseline[i:i+4] for i in range(0,len(baseline),4))
        assert changed>0,'flex did not change pixels'
    elif baseline is None:baseline=images[0];baseline_hashes=hashes;changed=0
    else:assert hashes==baseline_hashes,'disabled flex changed output';changed=0
    records.append(dict(enabled=enabled,workers=workers,prefetch=prefetch,frame_sha256=hashes,changed_pixels_vs_disabled=changed))
assert hashlib.sha256(executable.read_bytes()).hexdigest()==executable_hash,'executable changed during validation'
Path(f'outputs/sfm64-{prefix}-gpu-validation.json').write_text(json.dumps(dict(runs=records,workers_and_prefetch_pixel_exact=True,backward_seek_pixel_exact=True,scene_start=start,software_policy=args.software,executable_sha256=executable_hash,scope='Saved named controls; static descriptor history; candidate Vulkan renderer, native parity unverified'),indent=2)+'\n',encoding='utf-8')
print(f'Static flex GPU validation passed: {len(ticks)} animated samples per run, visible pixel changes, 1/4 workers and prefetch exact, backward seek and disabled output exact')
