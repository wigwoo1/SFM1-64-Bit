from pathlib import Path
import hashlib,subprocess,uuid,json
binary=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll');before=hashlib.sha256(binary.read_bytes()).hexdigest();assert before=='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
oracle=Path('outputs')/('elapsed-accumulator-'+uuid.uuid4().hex+'.bin');logs=[]
for arch,marker in [('x86','original_x86_tick_accumulator=65536'),('x64','x64_tick_accumulator_exact=65536')]:
 r=subprocess.run([str(Path('work')/('probe_elapsed_accumulator_'+arch+'.exe')),str(oracle)],capture_output=True,text=True);assert r.returncode==0,r.stdout+r.stderr;assert marker in r.stdout;logs.append(r.stdout.strip())
assert oracle.stat().st_size==65536*8*4;assert hashlib.sha256(binary.read_bytes()).hexdigest()==before
report=dict(cases=65536,binary_sha256=before,installation_unchanged=True,oracle=str(oracle),results=logs,limits='Original engine10182418..10182430 elapsed-add prefix plus1018243a..1018248e with return adapter; raw accumulator/elapsed/interval and retain branch, alternate alignment and signed current tick. Private mapped data explicitly supplied; no imports or initialization. Full scheduler, FP exception modes and playback integration remain unverified.')
Path('outputs/sfm64-elapsed-accumulator-native-validation.json').write_text(json.dumps(report,indent=2));print('\n'.join(logs))
