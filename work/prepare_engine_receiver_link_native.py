from pathlib import Path
import sys,json,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll');sha=hashlib.sha256(im.b).hexdigest();assert sha=='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
rows=json.loads(Path('outputs/sfm64-engine-shadow-manager-binding.json').read_text())['instructions'];header=['#pragma once'];manifest=[]
for name,start,end in [('unlink',0x10107f60,0x10107ff8),('insert',0x101092e0,0x10109354)]:
 code=im.b[im.offset(start):im.offset(start)+end-start];coverage=set();fixes=[]
 for row in rows:
  va=int(row['address'],16)
  if not start<=va<end:continue
  b=bytes.fromhex(row['bytes']);assert im.b[im.offset(va):im.offset(va)+len(b)]==b;coverage.update(range(va,va+len(b)))
  assert 'ds:[' not in row['instruction']
  if row['instruction'].startswith('call'):
   assert b[0]==0xe8;target=va+5+int.from_bytes(b[1:],'little',signed=True);assert target==0x10107f60;fixes.append(va-start+1)
 assert coverage==set(range(start,end))
 header.append('inline constexpr unsigned char '+name+'ReceiverBytes[]={'+','.join(hex(v) for v in code)+'};');manifest.append({'name':name,'size':len(code),'relative_unlink_call_offsets':fixes})
assert manifest[0]['relative_unlink_call_offsets']==[] and manifest[1]['relative_unlink_call_offsets']==[13]
Path('work/engine_receiver_link_native_bytes.h').write_text('\n'.join(header)+'\n')
Path('outputs/sfm64-engine-receiver-link-native-preparation.json').write_text(json.dumps({'engine_sha256':sha,'routines':manifest,'status':'Complete instruction coverage and single relative call relocation verified; execution pending.'},indent=2));print(manifest)
