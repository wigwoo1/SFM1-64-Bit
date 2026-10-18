from pathlib import Path
import os,re,subprocess,json,hashlib
log=Path('outputs/sfm64-timeline-trim-group-validation.txt').read_text();match=re.search(r'output=(work/trimmed-session-\d+-\d+\.dmx)',log);assert match
trimmed=match[1];assert Path(trimmed).is_file()
ticks=Path('work/document-trim-gpu-ticks.txt');ticks.write_text('871333\n871333\n871333\n');sessions=Path('work/document-trim-gpu-sessions.txt');sessions.write_text(f'work/roundtrip-11.dmx\n{trimmed}\nwork/roundtrip-11.dmx\n');runs=[];reference=None
for mode in (0,1):
 directory=Path(f'outputs/document-trim-prefetch-{mode}');directory.mkdir(exist_ok=True);env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')};env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CONTENT_DIRECTORY=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod',SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION='work/roundtrip-11.dmx',SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK='871333',SFM64_DIAGNOSTIC_SCENE_TICKS=str(ticks),SFM64_DIAGNOSTIC_SCENE_SESSION_SEQUENCE=str(sessions),SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_FRAME_PREFETCH=str(mode),SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
 run=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'],env=env,capture_output=True,text=True,timeout=60);text=run.stdout+run.stderr;Path(f'outputs/sfm64-document-trim-prefetch-{mode}.txt').write_text(text,encoding='utf-8');assert run.returncode==0,text
 pixels=[(directory/f'vulkan-retained-frame-{i}.rgba').read_bytes() for i in range(3)]
 assert pixels[0]==pixels[1]==pixels[2],'trim changed scene pixels'
 if reference is None:reference=pixels[0]
 assert pixels[0]==reference,'prefetch changed pixels'
 assert 'retained_gpu_material_cache_entries=1' in text
 runs.append(dict(prefetch=mode,pixels_exact=True,sha256=hashlib.sha256(pixels[0]).hexdigest()))
Path('outputs/sfm64-document-trim-gpu.json').write_text(json.dumps(dict(edited_session=trimmed,runs=runs,scope='saved session replacement, not interactive edit events'),indent=2),encoding='utf-8');print('document trim GPU passed original -> trimmed -> original, retained material cache, sync/prefetch pixels exact')
