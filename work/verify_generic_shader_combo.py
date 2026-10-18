from pathlib import Path
import subprocess,uuid,json,hashlib
root=Path(__file__).resolve().parent.parent
source=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\stdshader_dx9.dll')
recovery=json.loads((root/'outputs/sfm64-generic-shader-combo-packing-recovery.json').read_text())
digest=hashlib.sha256(source.read_bytes()).hexdigest()
if digest!=recovery['sha256']:raise RuntimeError('audited binary changed')
directory=root/'outputs'/('generic-combo-'+uuid.uuid4().hex);directory.mkdir();oracle=directory/'original.bin'
logs=[]
for architecture in ('x86','x64'):
 run=subprocess.run([str(root/f'work/probe_generic_shader_combo_{architecture}.exe'),str(oracle)],capture_output=True,text=True,check=True,timeout=60)
 logs.append(run.stdout.strip());print(logs[-1])
if hashlib.sha256(source.read_bytes()).hexdigest()!=digest:raise RuntimeError('installed binary changed')
if oracle.stat().st_size!=65536*21*4:raise RuntimeError('oracle size')
report={'native_x86_and_owned_x64_pairs':65536,'static_word_count':13,'dynamic_word_count':6,'cases':'basis vectors, all-bits basis, low-bit fields and deterministic full uint32 overflow inputs','result':logs,'source_sha256':digest,'installed_binary_unchanged':True,'oracle':str(oracle),'oracle_sha256':hashlib.sha256(oracle.read_bytes()).hexdigest(),'limits':'Executes only original audited leaf arithmetic in a private image without DLL entrypoint/import initialization. No full material draw, valid domain recovery, field ownership, command consumer or renderer parity is proved.'}
(root/'outputs/sfm64-generic-shader-combo-validation.json').write_text(json.dumps(report,indent=2)+'\n')
