import hashlib,json,subprocess,uuid
from pathlib import Path
binary=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
before=hashlib.sha256(binary.read_bytes()).hexdigest()
assert before=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
oracle=Path('outputs')/('generic-light-state-'+str(uuid.uuid4())+'.bin')
results=[]
for arch,marker in [('x86','original_x86_light_states=65536'),('x64','x64_native_light_states_exact=65536')]:
 result=subprocess.run([str(Path('work')/('probe_generic_light_state_'+arch+'.exe')),str(oracle)],capture_output=True,text=True,check=True)
 assert marker in result.stdout;results.append(result.stdout.strip())
assert oracle.stat().st_size==18*4+65536*99*4
assert hashlib.sha256(binary.read_bytes()).hexdigest()==before
report={'status':'passed','original_aggregation_cases':65536,'binary_sha256':before,'installation_unchanged':True,'oracle':str(oracle),'results':results,'limits':'Original aggregation executes over synthetic records using baseline bytes from the private uninitialized image. Runtime baseline population, record ownership, invalidation, full material policy and GPU behavior remain unverified.'}
report['native_cache_sequences']=65536
report['owned_cache_sequences']=65536
report['cache_sequence_scope']='Native cold/warm/explicit-validity-clear refresh with synthetic inputs; owned repeated query, explicit invalidation, and replacement with empty records. Draw routine itself is not executed.'
Path('outputs/sfm64-generic-light-state-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print('\n'.join(results))

