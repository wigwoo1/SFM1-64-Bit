import hashlib,json,subprocess,uuid
from pathlib import Path
binary=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
before=hashlib.sha256(binary.read_bytes()).hexdigest()
assert before=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
oracle=Path('outputs')/('generic-light-array-'+str(uuid.uuid4())+'.bin');results=[]
for arch,marker in [('x86','original_x86_light_arrays=65536'),('x64','x64_native_light_arrays_exact=65536')]:
 result=subprocess.run([str(Path('work')/('probe_generic_light_array_'+arch+'.exe')),str(oracle)],capture_output=True,text=True,check=True)
 assert marker in result.stdout;results.append(result.stdout.strip())
assert oracle.stat().st_size==65536*270*4
assert hashlib.sha256(binary.read_bytes()).hexdigest()==before
report={'status':'passed','cases':65536,'binary_sha256':before,'installation_unchanged':True,'oracle':str(oracle),'results':results,'limits':'Original setter/clear code with synthetic config and flush callback, original memcpy in private mapping. Covers signed count boundaries, all control bytes, copied prefix and retained tail. Real flush behavior, legal input domains, application integration and GPU lighting remain unverified.'}
Path('outputs/sfm64-generic-light-array-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print('\n'.join(results))

