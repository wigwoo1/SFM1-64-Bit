from pathlib import Path
import hashlib,subprocess,uuid,json
binary=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');before=hashlib.sha256(binary.read_bytes()).hexdigest();assert before=='8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4'
oracle=Path('outputs')/('time-snap-'+uuid.uuid4().hex+'.bin');logs=[]
for arch,marker in [('x86','original_x86_time_snap=65536'),('x64','x64_time_snap_exact=65536')]:
 r=subprocess.run([str(Path('work')/('probe_time_snap_'+arch+'.exe')),str(oracle)],capture_output=True,text=True);assert r.returncode==0,r.stdout+r.stderr;assert marker in r.stdout;logs.append(r.stdout.strip())
assert oracle.stat().st_size==65536*3*4;assert hashlib.sha256(binary.read_bytes()).hexdigest()==before
report=dict(cases=65536,binary_sha256=before,installation_unchanged=True,oracle=str(oracle),results=logs,limits='Original full ifm1097b020 mode0, private mapping without initialization/imports. Signed raw int32 ticks, nonzero uint16 packed rates, default FP controls. Other rounding modes, float rate constructor and live getter lifecycle unverified.')
Path('outputs/sfm64-time-snap-native-validation.json').write_text(json.dumps(report,indent=2));print('\n'.join(logs))
