import sys,hashlib,json,subprocess,uuid
from pathlib import Path
sys.path.insert(0,'work')
from pe_image import Image
path=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll');im=Image(path)
before=hashlib.sha256(im.b).hexdigest();assert before=='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
assert im.word(0x103666c4+29*4)==0x101e5670
arrays=[]
for name,va,size in [('nativeGate',0x101e5670,26),('nativeAccessor',0x100832d0,9),('nativeSetter',0x101a8fe0,13)]:
 o=im.offset(va);b=im.b[o:o+size];arrays.append('inline constexpr unsigned char '+name+'[]={'+','.join(hex(x) for x in b)+'};')
Path('work/document_engine_gate_bytes.h').write_text('#pragma once\n#include <cstring>\n'+'\n'.join(arrays))
subprocess.run(['cmd','/c','work\\build_document_engine_gate_probe.cmd'],check=True)
oracle=Path('outputs')/('document-engine-gate-'+uuid.uuid4().hex+'.bin')
a=subprocess.run(['work/probe_document_engine_gate_x86.exe',str(oracle)],check=True,capture_output=True,text=True)
assert oracle.stat().st_size==67328*12
b=subprocess.run(['work/probe_document_engine_gate_x64.exe',str(oracle)],check=True,capture_output=True,text=True)
assert 'native_engine_gate_matches=67328' in b.stdout
assert hashlib.sha256(path.read_bytes()).hexdigest()==before
report={'cases':67328,'native':a.stdout.strip(),'x64':b.stdout.strip(),'oracle':str(oracle),'engine_sha256':before,'installation_unchanged':True,'flag_setter_executed':True,'limitations':'Private copied setter/gate/accessor; relocated accessor call and global operand. Supplied tool/client objects; actual setter call-site lifecycle and live binding not established.'}
Path('outputs/sfm64-document-engine-gate-native-validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
