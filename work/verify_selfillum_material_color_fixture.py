"""Controlled RGB mask/HDR composition on actual native Vulkan frames."""
from pathlib import Path
import hashlib,json,os,subprocess,uuid
root=Path(__file__).resolve().parent.parent
exe=root/'build/session-core/probe_vulkan_win32_surface_revision_x64.exe'
source=root/'work/short-timeline-session.dmx'
before=hashlib.sha256(source.read_bytes()).hexdigest()
output=root/'outputs'/('selfillum-material-color-fixture-'+uuid.uuid4().hex[:8]);output.mkdir()
env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
env.update(SFM64_DIAGNOSTIC_MATERIAL_COLOR_TEST='1',SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_BOUNDARY_CONTROL_TEST='1',SFM64_DIAGNOSTIC_MASK_UPLOAD_TEST='1',SFM64_DIAGNOSTIC_SELFILLUM_TEST='1')
frames={}
for mode in ('black','red','white','alpha','alpha-half','red-half'):
 destination=output/mode;destination.mkdir()
 configured=dict(env,SFM64_DIAGNOSTIC_SELFILLUM_FIXTURE=mode,SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(destination))
 result=subprocess.run([str(exe),'--session',str(source),'--content-directory',r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod','--control-test','--geometry-workers','1'],cwd=root,env=configured,capture_output=True,text=True,timeout=60)
 log=result.stdout+result.stderr;(output/(mode+'.txt')).write_text(log)
 assert result.returncode==0,log
 assert 'base_texture_sampling=material_metadata mask_sampling=unorm' in log,log
 assert 'selfillum_shader_candidate=active' in log and 'sampled_by_shader=1' in log,log
 data=(destination/'vulkan-retained-frame-0.rgba').read_bytes();assert len(data)==512*512*4
 frames[mode]=data
# Compare identical covered pixels; skip background, edges and alpha-blended pixels.
covered=[i for i in range(0,len(frames['black']),4) if frames['black'][i:i+4]==bytes((13,55,134,255))]
assert len(covered)>100,'No sufficient constant covered surface'
expectations={'red-half':(8,55,134,255),'red':(3,55,134,255),'white':(3,55,255,255),'alpha':(3,55,255,255),'alpha-half':(8,55,255,128)}
for mode,expected in expectations.items():
 for i in covered:
  actual=frames[mode][i:i+4]
  assert all(abs(a-b)<=1 for a,b in zip(actual,expected)),(mode,i,list(actual),expected)
assert frames['alpha']==frames['white'],'Full base-alpha mask differs from full explicit RGB mask'
assert hashlib.sha256(source.read_bytes()).hexdigest()==before
report={'covered_pixels_compared':len(covered),'expected_rgba':expectations,'tolerance_codes':1,'frame_sha256':{mode:hashlib.sha256(data).hexdigest() for mode,data in frames.items()},'source_unchanged':True,'exe_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'scope':'Actual constant-texture explicit full/half RGB and full/partial base-alpha mask selection and tint composition with metadata-selected default sRGB base decoding and UNORM masks/output; white-mask HDR blue clamps to255. Source lighting/color-space fidelity, bloom and HDR output remain unverified.'}
(output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
