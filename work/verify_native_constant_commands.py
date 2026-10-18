from pathlib import Path
import subprocess,uuid,json,hashlib,struct
root=Path(__file__).resolve().parent.parent
source=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
digest=hashlib.sha256(source.read_bytes()).hexdigest()
recovery=json.loads((root/'outputs/sfm64-native-shader-constant-command-recovery.json').read_text())
if digest!=recovery['sha256']:raise RuntimeError('audited source changed')
directory=root/'outputs'/('native-constant-commands-'+uuid.uuid4().hex);directory.mkdir();oracle=directory/'original.bin';logs=[]
for architecture in ('x86','x64'):
 run=subprocess.run([str(root/f'work/probe_native_constant_commands_{architecture}.exe'),str(oracle)],capture_output=True,text=True,check=True,timeout=60)
 logs.append(run.stdout.strip());print(logs[-1])
data=oracle.read_bytes();assert len(data)==16384*152*4
skipped=0;pixel_prefix=0
for offset in range(0,len(data),152*4):
 words=struct.unpack_from('<152I',data,offset)
 skipped+=words[149]==0
 pixel_prefix+=words[65]==3 and words[149]>0 and words[150]>words[66]
if hashlib.sha256(source.read_bytes()).hexdigest()!=digest:raise RuntimeError('installed binary changed')
report={'native_x86_and_owned_x64_final_states':16384,'zero_device_callback_cases':skipped,'pixel_leading_register_skip_cases':pixel_prefix,'result':logs,'source_sha256':digest,'installed_binary_unchanged':True,'oracle':str(oracle),'oracle_sha256':hashlib.sha256(data).hexdigest(),'limits':'Original flat opcode3/4/0 dispatcher execution with synthetic device callbacks and owned x86 cache arrays. X64 final state and callback count/start/count match the native oracle. Invalidation and failed-callback behavior are owned policies. No queued path, specialized commands, real GPU calls or complete material rendering.'}
(root/'outputs/sfm64-native-constant-command-validation.json').write_text(json.dumps(report,indent=2)+'\n');print('zero_callbacks='+str(skipped)+' pixel_prefix_skips='+str(pixel_prefix))
