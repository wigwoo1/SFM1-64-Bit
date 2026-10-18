"""Authored gamma-read controls and mixed texture formats on actual GPU."""
from pathlib import Path
import hashlib,json,os,subprocess,uuid
root=Path(__file__).resolve().parent.parent
exe=root/'build/session-core/probe_vulkan_win32_surface_revision_x64.exe'
source=root/'work/short-timeline-session.dmx';before=hashlib.sha256(source.read_bytes()).hexdigest()
output=root/'outputs'/('mixed-material-color-'+uuid.uuid4().hex[:8]);output.mkdir()
env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_BOUNDARY_CONTROL_TEST='1',SFM64_DIAGNOSTIC_MASK_UPLOAD_TEST='1',SFM64_DIAGNOSTIC_SELFILLUM_TEST='1',SFM64_DIAGNOSTIC_MATERIAL_COLOR_TEST='1',SFM64_DIAGNOSTIC_SELFILLUM_FIXTURE='black')
frames={}
for mode in ('0','1','mixed'):
 destination=output/mode;destination.mkdir()
 configured=dict(env,SFM64_DIAGNOSTIC_FIXTURE_GAMMA_READ=mode,SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(destination))
 result=subprocess.run([str(exe),'--session',str(source),'--content-directory',r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod','--control-test','--geometry-workers','1'],cwd=root,env=configured,capture_output=True,text=True,timeout=60)
 log=result.stdout+result.stderr;(output/(mode+'.txt')).write_text(log)
 assert result.returncode==0 and 'base_texture_sampling=material_metadata' in log,log
 frames[mode]=(destination/'vulkan-retained-frame-0.rgba').read_bytes();assert len(frames[mode])==512*512*4
srgb=bytes((13,55,134,255));unorm=bytes((64,128,192,255))
covered=[i for i in range(0,len(frames['0']),4) if frames['0'][i:i+4]==srgb]
assert len(covered)>100
counts={'srgb':0,'unorm':0}
for i in covered:
 assert frames['1'][i:i+4]==unorm,(i,list(frames['1'][i:i+4]))
 value=frames['mixed'][i:i+4];assert value in (srgb,unorm),(i,list(value))
 counts['srgb' if value==srgb else 'unorm']+=1
assert min(counts.values())>100,counts
assert hashlib.sha256(source.read_bytes()).hexdigest()==before
report={'covered_pixels':len(covered),'mixed_sampling_pixel_counts':counts,'frame_sha256':{mode:hashlib.sha256(data).hexdigest() for mode,data in frames.items()},'source_unchanged':True,'exe_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'scope':'Owned authored VMT gamma-read values parsed to material metadata and GPU texture formats; actual mixed-format frame verified. Original Source material/shader parity, output-policy mixing and lighting unfinished.'}
(output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
