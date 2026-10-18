import hashlib,json,subprocess,uuid
from pathlib import Path
binary=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
before=hashlib.sha256(binary.read_bytes()).hexdigest()
assert before=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
oracle=Path('outputs')/('pending-flush-'+str(uuid.uuid4())+'.bin');results=[]
for arch,marker in [('x86','original_x86_pending_flushes=65536'),('x64','x64_native_pending_flushes_exact=65536')]:
 result=subprocess.run([str(Path('work')/('probe_pending_draw_flush_'+arch+'.exe')),str(oracle)],capture_output=True,text=True,check=True)
 assert marker in result.stdout;results.append(result.stdout.strip())
assert oracle.stat().st_size==65536*10*4
assert hashlib.sha256(binary.read_bytes()).hexdigest()==before
report={'status':'passed','cases':65536,'binary_sha256':before,'installation_unchanged':True,'oracle':str(oracle),'results':results,'limits':'Synthetic ordinary/alternate/finish callbacks, original private dispatcher. Covers null endpoint, guard0/1, all pending bytes, alternate0..63, arguments, callback state and reentry suppression. Interface replacement during callback, exception behavior and rendering bodies remain unverified.'}
Path('outputs/sfm64-pending-flush-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print('\n'.join(results))

