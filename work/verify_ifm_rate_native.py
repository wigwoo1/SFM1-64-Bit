from pathlib import Path
import hashlib,subprocess,uuid,json
binary=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');before=hashlib.sha256(binary.read_bytes()).hexdigest();assert before=='8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4'
oracle=Path('outputs')/('ifm-rate-'+uuid.uuid4().hex+'.bin');logs=[]
for arch,marker in [('x86','original_x86_ifm_rate=65536'),('x64','x64_ifm_rate_exact=65536')]:
 r=subprocess.run([str(Path('work')/('probe_ifm_rate_'+arch+'.exe')),str(oracle)],capture_output=True,text=True);assert r.returncode==0,r.stdout+r.stderr;assert marker in r.stdout;logs.append(r.stdout.strip())
assert oracle.stat().st_size==65536*2*4;assert hashlib.sha256(binary.read_bytes()).hexdigest()==before
report=dict(cases=65536,binary_sha256=before,installation_unchanged=True,oracle=str(oracle),results=logs,limits='Original full ifm1097b900 constructor, private mapping without initialization/imports. Finite nonnegative rates0..65535, default nearest x87/MXCSR controls. Invalid rates, other rounding controls and live getter lifecycle unverified.')
Path('outputs/sfm64-ifm-rate-native-validation.json').write_text(json.dumps(report,indent=2));print('\n'.join(logs))
