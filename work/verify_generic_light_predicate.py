import hashlib,json,subprocess,uuid
from pathlib import Path
binary=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\stdshader_dx9.dll')
before=hashlib.sha256(binary.read_bytes()).hexdigest()
assert before=='fb09e2ddc8a99aff4ef31e3fddfdb1ca84d89379336abdab4e76e94adfa7e6f8'
oracle=Path('outputs')/('generic-lights-'+str(uuid.uuid4())+'.bin')
results=[]
for arch,marker in [('x86','original_x86_light_predicates=65536'),('x64','x64_native_light_predicates_exact=65536 scoped_field_cases=45')]:
 result=subprocess.run([str(Path('work')/('probe_generic_light_predicate_'+arch+'.exe')),str(oracle)],capture_output=True,text=True,check=True)
 assert marker in result.stdout;results.append(result.stdout.strip())
assert oracle.stat().st_size==65536*3*4
assert hashlib.sha256(binary.read_bytes()).hexdigest()==before
report={'status':'passed','original_predicate_cases':65536,'scoped_owned_field_cases':45,'binary_sha256':before,'installation_unchanged':True,'oracle':str(oracle),'results':results,'limits':'Native execution covers light predicate only, all256 flag-byte values and signed boundaries. Field assembly is supported by checked branch instructions and owned assertions, not whole-helper execution. Legal rendering domains and branch selection remain unresolved.'}
Path('outputs/sfm64-generic-light-predicate-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print('\n'.join(results))
