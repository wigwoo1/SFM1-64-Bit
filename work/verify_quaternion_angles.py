import hashlib,json,subprocess,uuid
from pathlib import Path
binary=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
before=hashlib.sha256(binary.read_bytes()).hexdigest()
assert before=='8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4'
oracle=Path('outputs')/('quaternion-angles-'+str(uuid.uuid4())+'.bin')
results=[]
for arch,marker in [('x86','original_x86_quaternion_angles=65536'),('x64','x64_quaternion_angles_exact=65536')]:
 result=subprocess.run([str(Path('work')/('probe_quaternion_angles_'+arch+'.exe')),str(oracle)],capture_output=True,text=True,check=True)
 assert marker in result.stdout;results.append(result.stdout.strip())
assert oracle.stat().st_size==65536*7*4
assert hashlib.sha256(binary.read_bytes()).hexdigest()==before
report={'status':'passed','cases':65536,'binary_sha256':before,'installation_unchanged':True,'oracle':str(oracle),'results':results,'limits':'Finite quaternion components in [-1,1]; original wrapper and bounded math helper, default FP. Other quaternion domains and full jitter/renderer parity remain unverified.'}
Path('outputs/sfm64-generic-quaternion-angles-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print('\n'.join(results))
