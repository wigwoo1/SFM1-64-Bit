from pathlib import Path
import hashlib,subprocess,uuid,json
binary=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll');before=hashlib.sha256(binary.read_bytes()).hexdigest();assert before=='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
oracle=Path('outputs')/('drift-mean-'+uuid.uuid4().hex+'.bin');logs=[]
for arch,marker in [('x86','original_x86_drift_mean=65536'),('x64','x64_drift_mean_exact=65536')]:
 r=subprocess.run([str(Path('work')/('probe_drift_mean_'+arch+'.exe')),str(oracle)],capture_output=True,text=True);assert r.returncode==0,r.stdout+r.stderr;assert marker in r.stdout;logs.append(r.stdout.strip())
assert oracle.stat().st_size==65536*18*4;assert hashlib.sha256(binary.read_bytes()).hexdigest()==before
report=dict(cases=65536,binary_sha256=before,installation_unchanged=True,oracle=str(oracle),results=logs,limits='Original engine10143820 mean plus x87 interval multiplication and final float store. Raw sixteen history words/interval, default masked FP controls; inputs preserved, no imports or initialization. Full correction and playback integration remain unverified.')
Path('outputs/sfm64-drift-mean-native-validation.json').write_text(json.dumps(report,indent=2));print('\n'.join(logs))
