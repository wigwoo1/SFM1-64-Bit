from pathlib import Path
import subprocess,json
s=Path('work/build_alternate_geometry_inspector.cmd').read_text().replace('inspect_alternate_geometry','test_vtf73_texture');Path('work/build_vtf73_texture_test.cmd').write_text(s)
r=subprocess.run(['cmd.exe','/d','/c','work\\build_vtf73_texture_test.cmd'],capture_output=True,text=True);assert r.returncode==0,r.stdout+r.stderr
records=json.loads(Path('outputs/sfm64-failing-texture-header-audit.json').read_text());passed=[]
for item in records:
 r=subprocess.run(['build/session-core/test_vtf73_texture.exe',item['texture']],capture_output=True,text=True);assert r.returncode==0,r.stderr
 passed.append({'texture':item['texture'],'validation':r.stdout.strip()})
Path('outputs/sfm64-vtf73-texture-validation.json').write_text(json.dumps(passed,indent=2)+'\n');print('VTF73 real textures decoded at mip zero; truncated resource tables rejected; cases='+str(len(passed)))
