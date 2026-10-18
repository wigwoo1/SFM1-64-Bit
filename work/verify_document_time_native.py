from pathlib import Path
import hashlib,subprocess,uuid,json
binary=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');before=hashlib.sha256(binary.read_bytes()).hexdigest();assert before=='8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4'
oracle=Path('outputs')/('document-time-'+uuid.uuid4().hex+'.bin');logs=[]
for arch,marker in [('x86','original_x86_document_time=65536'),('x64','x64_document_time_exact=65536')]:
 r=subprocess.run([str(Path('work')/('probe_document_time_'+arch+'.exe')),str(oracle)],capture_output=True,text=True);assert r.returncode==0,r.stdout+r.stderr;assert marker in r.stdout;logs.append(r.stdout.strip())
assert oracle.stat().st_size==65536*14*4;assert hashlib.sha256(binary.read_bytes()).hexdigest()==before
report=dict(cases=65536,binary_sha256=before,installation_unchanged=True,oracle=str(oracle),results=logs,limits='Original full ifm1029a950 and10275640 compared with owned x64 monotonic conversion, controlled engine gate/current float/retained float and owner fields. Flag6c0=false excludes settings/snapping. Default masked FP; real owner lifecycle and live playback unfinished.')
Path('outputs/sfm64-document-time-native-validation.json').write_text(json.dumps(report,indent=2));print('\n'.join(logs))
