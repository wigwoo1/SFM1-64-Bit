import hashlib,json,subprocess,uuid
from pathlib import Path
binary=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\stdshader_dx9.dll')
before=hashlib.sha256(binary.read_bytes()).hexdigest()
assert before=='fb09e2ddc8a99aff4ef31e3fddfdb1ca84d89379336abdab4e76e94adfa7e6f8'
oracle=Path('outputs')/('generic-initializers-'+str(uuid.uuid4())+'.bin')
results=[]
for arch,marker in [('x86','original_x86_initializer_pairs=65536'),('x64','x64_native_initializer_pairs_exact=65536')]:
 result=subprocess.run([str(Path('work')/('probe_generic_shader_initializers_'+arch+'.exe')),str(oracle)],capture_output=True,text=True,check=True)
 assert marker in result.stdout;results.append(result.stdout.strip())
assert oracle.stat().st_size==65536*23*4
assert hashlib.sha256(binary.read_bytes()).hexdigest()==before
report={'status':'passed','cases':65536,'binary_sha256':before,'installation_unchanged':True,'oracle':str(oracle),'results':results,'limits':'Original constructors with synthetic interface callbacks in a private mapping. No original DLL initialization or GPU runtime. Full material branch policy and implicit field semantic names remain unverified.'}
Path('outputs/sfm64-generic-shader-initializers-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print('\n'.join(results))
