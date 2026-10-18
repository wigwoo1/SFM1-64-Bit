from pathlib import Path
import sys,json,hashlib,re,subprocess
sys.path.insert(0,'work')
from pe_image import Image
binary=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\materialsystem.dll');im=Image(binary)
recovery=json.loads(Path('outputs/sfm64-material-call-queue-recovery.json').read_text());before=hashlib.sha256(im.b).hexdigest();assert before==recovery['materialsystem_sha256']
rows=[row for row in recovery['instructions'] if 0x100228f0<=int(row['address'],16)<0x10022908]
raw=b''.join(bytes.fromhex(row['bytes']) for row in rows);assert len(raw)==24 and raw==im.b[im.offset(0x100228f0):im.offset(0x100228f0)+24]
header='#pragma once\nstatic constexpr unsigned char drawInvokeBytes[]={'+','.join(map(str,raw))+'};\n'
assert Path('work/material_draw_invoke_native_bytes.h').read_text()==header
runs=[]
for bits,name in [(32,'test_material_draw_invoke_native.exe'),(64,'test_material_draw_invoke_candidate_64.exe')]:
 output=subprocess.run([str(Path('work',name).resolve())],capture_output=True,text=True,check=True).stdout.strip()
 fields={k:int(v) for k,v in re.findall(r'(draw_invoke_cases|pointer_bits|digest)=(\d+)',output)}
 assert fields['draw_invoke_cases']==65536 and fields['pointer_bits']==bits
 runs.append({'result':fields,'output':output,'executable_sha256':hashlib.sha256(Path('work',name).read_bytes()).hexdigest()})
assert runs[0]['result']['digest']==runs[1]['result']['digest']
assert hashlib.sha256(binary.read_bytes()).hexdigest()==before
report={'materialsystem_sha256':before,'installed_binary_unchanged':True,'native_invoke_bytes':24,'runs':runs,'limitations':'Only invoke body executed natively; callback substituted. Candidate release flags0 observation does not execute native release/destructor. Native construction, queue draw receiver GPU and public x64 ABI remain unverified.'}
Path('outputs/sfm64-material-draw-invoke-native-validation.json').write_text(json.dumps(report,indent=2))
print('draw invoke native/x64 comparison65536 cases each passed; installedSHA unchanged')
