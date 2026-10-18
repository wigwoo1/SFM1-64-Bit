import hashlib,json,subprocess,uuid
from pathlib import Path
binary=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
before=hashlib.sha256(binary.read_bytes()).hexdigest()
assert before=='8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4'
oracle=Path('outputs')/('angle-basis-'+str(uuid.uuid4())+'.bin')
results=[]
for arch,marker in [('x86','original_x86_angle_bases=65536'),('x64','x64_basis_products_exact=65536')]:
 result=subprocess.run([str(Path('work')/('probe_angle_basis_'+arch+'.exe')),str(oracle)],capture_output=True,text=True,check=True)
 assert marker in result.stdout;results.append(result.stdout.strip())
assert "x64_complete_angle_bases_exact=65536" in results[-1]
assert "x64_angle_domain_rejections=5" in results[-1]
assert oracle.stat().st_size==65536*18*4
assert hashlib.sha256(binary.read_bytes()).hexdigest()==before
report={'status':'passed','cases':65536,'binary_sha256':before,'installation_unchanged':True,'oracle':str(oracle),'results':results,'limits':'Finite angle inputs within +/-36000 degrees, original-rounded FSINCOS trig supplied to owned arithmetic. Complete owned x64 FSINCOS angle-to-basis matches in this finite domain. Quaternion-angle conversion and renderer jitter remain unverified.'}
Path('outputs/sfm64-generic-angle-basis-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print('\n'.join(results))
