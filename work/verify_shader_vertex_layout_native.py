from pathlib import Path
import hashlib,json,re,subprocess
binary=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
before=hashlib.sha256(binary.read_bytes()).hexdigest()
expected=json.loads(Path('outputs/sfm64-shader-vertex-layout-recovery.json').read_text())['sha256']
assert before==expected
rows=[]
for architecture,executable in [(32,'test_shader_vertex_layout_native.exe'),(64,'test_shader_vertex_layout_candidate_64.exe')]:
 result=subprocess.run([str(Path('work',executable).resolve())],capture_output=True,text=True,check=True)
 fields={key:int(value) for key,value in re.findall(r'(layout_cases|normalized_digest|pointer_bits|multichannel_cases|descriptor_fields)=(\d+)',result.stdout)}
 assert fields['pointer_bits']==architecture and fields['layout_cases']==4435720 and fields['multichannel_cases']==3125000
 rows.append({'architecture':architecture,'result':fields,'output':result.stdout.strip(),'exe_sha256':hashlib.sha256(Path('work',executable).read_bytes()).hexdigest()})
assert rows[0]['result']['normalized_digest']==rows[1]['result']['normalized_digest']
after=hashlib.sha256(binary.read_bytes()).hexdigest();assert after==before
report={'installed_shaderapidx9_sha256':before,'installed_binary_unchanged':True,'runs':rows,'limitations':'Original routine executes only in x86 private mapped copy; x64 result normalization compares candidate fields. Native security check substituted, globals owned by fixture. Texture dimensions0..4 only; no GPU or public ABI validation.'}
Path('outputs/sfm64-shader-vertex-layout-native-validation.json').write_text(json.dumps(report,indent=2))
Path('outputs/sfm64-shader-vertex-layout-native-validation.txt').write_text('\n'.join(row['output'] for row in rows)+'\n')
print('validated4435720 cases per architecture, identical normalized fields, installedSHA unchanged')
