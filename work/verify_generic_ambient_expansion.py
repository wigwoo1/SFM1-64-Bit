import hashlib,json,subprocess,uuid
from pathlib import Path
binary=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
before=hashlib.sha256(binary.read_bytes()).hexdigest()
assert before=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
oracle=Path('outputs')/('ambient-expansion-'+str(uuid.uuid4())+'.bin')
results=[]
for arch,marker in [('x86','original_x86_ambient_expansions=65536'),('x64','x64_native_ambient_expansions_exact=65536')]:
 result=subprocess.run([str(Path('work')/('probe_generic_ambient_expansion_'+arch+'.exe')),str(oracle)],capture_output=True,text=True,check=True)
 assert marker in result.stdout;results.append(result.stdout.strip())
assert oracle.stat().st_size==65536*43*4
assert hashlib.sha256(binary.read_bytes()).hexdigest()==before
report={'status':'passed','cases':65536,'binary_sha256':before,'installation_unchanged':True,'oracle':str(oracle),'results':results,'limits':'Original expansion only, masked x87 exceptions. Full upload dispatch and GPU rendering remain unverified. Count argument is varied but unused by recovered routine.'}
Path('outputs/sfm64-generic-ambient-expansion-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print('\n'.join(results))
