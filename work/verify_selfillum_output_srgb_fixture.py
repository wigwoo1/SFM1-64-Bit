"""Controlled RGB mask/HDR composition on actual native Vulkan frames."""
from pathlib import Path
import hashlib,json,os,subprocess,uuid
root=Path(__file__).resolve().parent.parent
exe=root/'build/session-core/probe_vulkan_win32_surface_revision_x64.exe'
source=root/'work/short-timeline-session.dmx'
before=hashlib.sha256(source.read_bytes()).hexdigest()
output=root/'outputs'/('selfillum-output-srgb-fixture-'+uuid.uuid4().hex[:8]);output.mkdir()
env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
env.update(SFM64_DIAGNOSTIC_OUTPUT_SRGB_TEST='1',SFM64_DIAGNOSTIC_BASE_SRGB_TEST='1',SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_BOUNDARY_CONTROL_TEST='1',SFM64_DIAGNOSTIC_MASK_UPLOAD_TEST='1',SFM64_DIAGNOSTIC_SELFILLUM_TEST='1')
frames={}
for mode in ('black','red','white','alpha','alpha-half','red-half'):
 destination=output/mode;destination.mkdir()
 configured=dict(env,SFM64_DIAGNOSTIC_SELFILLUM_FIXTURE=mode,SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(destination))
 result=subprocess.run([str(exe),'--session',str(source),'--content-directory',r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod','--control-test','--geometry-workers','1'],cwd=root,env=configured,capture_output=True,text=True,timeout=60)
 log=result.stdout+result.stderr;(output/(mode+'.txt')).write_text(log)
 assert result.returncode==0,log
 assert 'base_texture_sampling=srgb mask_sampling=unorm' in log and 'output_attachment_encoding=srgb alpha_encoding=linear' in log,log
 assert 'selfillum_shader_candidate=active' in log and 'sampled_by_shader=1' in log,log
 data=(destination/'vulkan-retained-frame-0.rgba').read_bytes();assert len(data)==512*512*4
 frames[mode]=data
# Compare identical covered pixels; skip background, edges and alpha-blended pixels.
covered=[i for i in range(0,len(frames['black']),4) if frames['black'][i:i+4]==bytes((64,128,192,255))]
assert len(covered)>100,'No sufficient constant covered surface'
def decode(value):
 x=value/255.0
 return x/12.92 if x<=.04045 else ((x+.055)/1.055)**2.4
def encode(value):
 x=max(0.0,min(1.0,value))
 return round(255*(12.92*x if x<=.0031308 else 1.055*x**(1/2.4)-.055))
tint=json.loads((root/'outputs/native-fixed-gamma22-table-validation.json').read_text())['midpoint_index128']
base=[decode(v) for v in (64,128,192)]
expectations={}
for mode,mask,alpha in [('red-half',(128/255,0,0),255),('red',(1,0,0),255),('white',(1,1,1),255),('alpha',(1,1,1),255),('alpha-half',(128/255,)*3,128)]:
 expectations[mode]=tuple(encode(color+weight*(factor*color-color)) for color,weight,factor in zip(base,mask,(tint,1,5)))+(alpha,)

for mode,expected in expectations.items():
 for i in covered:
  actual=frames[mode][i:i+4]
  assert all(abs(a-b)<=1 for a,b in zip(actual,expected)),(mode,i,list(actual),expected)
assert frames['alpha']==frames['white'],'Full base-alpha mask differs from full explicit RGB mask'
assert hashlib.sha256(source.read_bytes()).hexdigest()==before
report={'covered_pixels_compared':len(covered),'expected_rgba':expectations,'tolerance_codes':1,'frame_sha256':{mode:hashlib.sha256(data).hexdigest() for mode,data in frames.items()},'source_unchanged':True,'exe_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'scope':'Actual constant-texture explicit full/half RGB and full/partial base-alpha mask selection and tint composition with sRGB base decoding, UNORM masks and sRGB output encoding; white-mask HDR blue clamps to255. Source lighting/color-space fidelity, bloom and HDR output remain unverified.'}
(output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
