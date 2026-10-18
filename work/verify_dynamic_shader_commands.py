from pathlib import Path
import subprocess,uuid,json,hashlib
root=Path(__file__).resolve().parent.parent
source=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
recovery=json.loads((root/'outputs/sfm64-native-dynamic-shader-command-recovery.json').read_text())
digest=hashlib.sha256(source.read_bytes()).hexdigest()
if digest!=recovery['binaries'][str(source)]:raise RuntimeError('audited binary changed')
directory=root/'outputs'/('dynamic-commands-'+uuid.uuid4().hex);directory.mkdir();oracle=directory/'original.bin'
logs=[]
for architecture in ('x86','x64'):
 run=subprocess.run([str(root/f'work/probe_dynamic_shader_commands_{architecture}.exe'),str(oracle)],capture_output=True,text=True,check=True,timeout=60)
 logs.append(run.stdout.strip());print(logs[-1])
if hashlib.sha256(source.read_bytes()).hexdigest()!=digest:raise RuntimeError('installed binary changed')
if oracle.stat().st_size!=65536*21*4:raise RuntimeError('oracle size')
report={'native_x86_and_owned_x64_pairs':65536,'stream_words':7,'supported_opcodes':[0,11,12],'cases':'three selection commands, all eight stage orders, deterministic initial state/operands, unrelated manager storage preservation','result':logs,'source_sha256':digest,'installed_binary_unchanged':True,'oracle':str(oracle),'oracle_sha256':hashlib.sha256(oracle.read_bytes()).hexdigest(),'limits':'Executes original dispatcher restricted to audited selection commands with a synthetic manager global in a private image without DLL entrypoint/import initialization. No other opcodes, pointer/nested streams, material draw or GPU binding is proved. Owned parser rejects unsupported/truncated/trailing streams before returning changed state.'}
(root/'outputs/sfm64-dynamic-shader-command-validation.json').write_text(json.dumps(report,indent=2)+'\n')
