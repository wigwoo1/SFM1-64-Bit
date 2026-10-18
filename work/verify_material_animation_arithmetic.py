from pathlib import Path
import hashlib,subprocess,uuid,json
binary=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf\bin\client.dll');before=hashlib.sha256(binary.read_bytes()).hexdigest();assert before=='91dd1062227b095f1338312aa7268121894970c3e0a5b0d314e42c77f2aba73b'
oracle=Path('outputs')/('material-arithmetic-'+uuid.uuid4().hex+'.bin');logs=[]
for arch,marker in [('x86','original_x86_material_frames=65536'),('x64','x64_material_frames_exact=65536')]:
 r=subprocess.run([str(Path('work')/('probe_material_animation_arithmetic_'+arch+'.exe')),str(oracle)],capture_output=True,text=True);assert r.returncode==0,r.stdout+r.stderr;assert marker in r.stdout;logs.append(r.stdout.strip())
assert oracle.stat().st_size==65536*7*4;assert hashlib.sha256(binary.read_bytes()).hexdigest()==before
report=dict(cases=65536,binary_sha256=before,installation_unchanged=True,oracle=str(oracle),results=logs,limits='Original contiguous arithmetic1000f16a..1000f1b0 copied with return adapter. Both current/previous signed remainders; raw clock/start/duration/rate bits, counts1..127. No native callback/no-wrap branch or full bind execution; clock producer and GPU timeline integration remain unverified.')
Path('outputs/sfm64-material-animation-arithmetic-validation.json').write_text(json.dumps(report,indent=2));print('\n'.join(logs))
