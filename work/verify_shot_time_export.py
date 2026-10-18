from pathlib import Path
import subprocess,json,hashlib
root=Path.cwd()
exe=root/'build/session-core/export_shot_render_inputs_x64.exe'
session=root/'work/roundtrip-11.dmx'
mount=Path('D:/SteamLibrary/steamapps/common/SourceFilmmaker/game/usermod')
direct=root/'work/shot-render-2470-v1'
timeline=root/'work/shot-render-tick-870833-v1'
checks={}
for name in ('camera.bin','model-0-placement.bin','model-0-pose.bin'):
 a=(direct/name).read_bytes();b=(timeline/name).read_bytes()
 assert a==b,name
 checks[name]={'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest(),'direct_export_equal':True}
manifest=(timeline/'manifest.txt').read_text()
assert 'clip=2470 camera=2481' in manifest
assert 'root_parent_tick=870833 selected_local_tick=0' in manifest
rejected=[]
for tick in ('0','1174550','3400000','2147483648','12abc'):
 output=root/f'work/shot-time-reject-{tick}'
 assert not output.exists()
 result=subprocess.run([str(exe),str(session),'2237',str(mount),str(output),tick],capture_output=True,text=True)
 assert result.returncode!=0 and not output.exists(),(tick,result.stdout,result.stderr)
 rejected.append({'tick':tick,'error':result.stderr.strip(),'output_created':False})
report={'checks':checks,'rejected':rejected,'selection_policy':'candidate half-open intervals; ambiguous shots rejected','native_selection_parity':'unverified','animation_sampled':False}
(root/'outputs/sfm64-film-shot-time-export-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print('timeline export validated: 3 files byte-exact, 5 invalid or ambiguous selections rejected without output')
