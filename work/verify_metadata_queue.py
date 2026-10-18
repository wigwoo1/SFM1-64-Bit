import hashlib,json,subprocess,uuid
from pathlib import Path
binary=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
before=hashlib.sha256(binary.read_bytes()).hexdigest()
assert before=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
oracle=Path('outputs')/('metadata-queue-'+str(uuid.uuid4())+'.bin')
results=[]
for arch,marker in [('x86','original_x86_metadata_queues=65536'),('x64','x64_native_metadata_queues_exact=65536')]:
 result=subprocess.run([str(Path('work')/('probe_metadata_queue_'+arch+'.exe')),str(oracle)],capture_output=True,text=True,check=True)
 assert marker in result.stdout;results.append(result.stdout.strip())
assert oracle.stat().st_size==65536*72*4
assert hashlib.sha256(binary.read_bytes()).hexdigest()==before
report={'status':'passed','cases':65536,'binary_sha256':before,'installation_unchanged':True,'oracle':str(oracle),'results':results,'limits':'Original queued BOOL/INT4 methods, initialized caches, force and identical cases, bounded index/counts, two cache copies and queue tails. Refill, direct GPU device path and complete renderer remain unverified.'}
Path('outputs/sfm64-generic-metadata-queue-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print('\n'.join(results))
