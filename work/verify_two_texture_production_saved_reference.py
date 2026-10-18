"""Current Vulkan production path versus immutable previously live D3D9 captures.

This intentionally does not launch D3D9 or claim a fresh live-backend comparison.
"""
from pathlib import Path
import hashlib,json,struct,subprocess,tempfile

root=Path(__file__).resolve().parent.parent
out=root/'outputs/unlit-two-texture-gpu-20261014'
reference=out/'comparison-wrijpqjo'
prior=json.loads((reference/'results.json').read_text())
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def require(condition,message):
 if not condition:raise RuntimeError(message)
cases={entry['case']:entry for entry in prior['cases'] if entry['backend']=='vulkan'}
require(len(cases)==14,'Saved D3D9 reference case count')
for name,entry in cases.items():
 require(digest(reference/'d3d9'/(name+'.rgba'))==entry['d3d9_sha256'],'Saved reference bytes changed')
exe=out/'probe_unlit_two_texture_vulkan.exe'
raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
require(struct.unpack_from('<H',raw,pe+4)[0]==0x8664 and struct.unpack_from('<H',raw,pe+24)[0]==0x20b,'AMD64 PE32+ required')
directory=Path(tempfile.mkdtemp(prefix='production-reference-',dir=out))
run=subprocess.run([str(exe),str(out),str(directory/'captures'),'--production-transport'],cwd=root,
                   capture_output=True,text=True,timeout=45)
(directory/'runtime.log').write_text(run.stdout+run.stderr)
require(run.returncode==0 and 'production_fragment_and_shared_indexed_recorder=passed' in run.stdout,'Production GPU execution failed')
records=[]
for name,entry in cases.items():
 source=(reference/'d3d9'/(name+'.rgba')).read_bytes()
 current=(directory/'captures'/(name+'.rgba')).read_bytes()
 require(len(source)==len(current)==64*64*4,'Capture size differs')
 errors=[abs(a-b) for a,b in zip(source,current)]
 require(max(errors)<= (2 if name.startswith('srgb-') else 1),'Production/reference mismatch: '+name)
 records.append(dict(case=name,maximum_channel_error=max(errors),
                     differing_channels=sum(value!=0 for value in errors),
                     reference_sha256=entry['d3d9_sha256'],current_sha256=hashlib.sha256(current).hexdigest()))
report=dict(status='PASS_CURRENT_VULKAN_VS_SAVED_D3D9',executable=str(exe),executable_sha256=digest(exe),
 reference_receipt=str(reference/'results.json'),reference_receipt_sha256=digest(reference/'results.json'),
 reference_d3d9_executable=prior['runs']['d3d9'],cases=records,compared_pixels=14*64*64,
 source_sha256=digest(root/'work/sfm64-unlit-two-texture-candidate.hlsl'),
 spirv_sha256=digest(out/'sfm64-unlit-two-texture-candidate.spv'),
 limitations=['D3D9 captures are historical; fresh D3D9 device creation currently fails.',
              'White modulation/unit light scale, fixed UV/point filtering and normal uncloaked subset only.',
              'Shared indexed recorder and production fragment exercised, diagnostic vertex shader remains synthetic.',
              'No editor default activation, whole-material, proxy, fog/cloak or full-scene parity claimed.'])
(directory/'results.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS: current production Vulkan/shared recorder versus 14 saved D3D9 cases; 57,344 pixels; maximum error='+str(max(record['maximum_channel_error'] for record in records)))
print(directory/'results.json')
