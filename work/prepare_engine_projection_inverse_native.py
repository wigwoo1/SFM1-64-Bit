from pathlib import Path
import sys,json,hashlib,re
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll');sha=hashlib.sha256(im.b).hexdigest();assert sha=='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
rows=json.loads(Path('outputs/sfm64-engine-shadow-manager-binding.json').read_text())['instructions']
routines=[('inverse',0x1024a580,0x1024a8b6)]
starts={a for _,a,_ in routines};constants={0x1031bdec:0x3f800000,0x103717a0:0x7fffffff,0x1031cbb0:0x80000000,0x10350f0c:0x358637bd,0x10415d00:im.word(0x10415d00)}
for address,bits in constants.items():assert im.word(address)==bits
manifest=[];header=['#pragma once']
for name,a,z in routines:
 code=im.b[im.offset(a):im.offset(a)+z-a];selected=[r for r in rows if a<=int(r['address'],16)<z];assert selected
 covered=set()
 for r in selected:
  va=int(r['address'],16);b=bytes.fromhex(r['bytes']);assert im.b[im.offset(va):im.offset(va)+len(b)]==b
  covered.update(range(va,va+len(b)))
 assert covered==set(range(a,z)),name
 fixes=[]
 for r in selected:
  va=int(r['address'],16);b=bytes.fromhex(r['bytes']);op=r['instruction']
  if 'ds:[' in op:
   target=int(op.split('ds:[')[1].split('h')[0],16);assert target in constants;assert int.from_bytes(b[-4:],'little')==target
   fixes.append({'offset':va-a+len(b)-4,'kind':'absolute','target':hex(target)})
  if op.startswith('call'):
   assert b[0]==0xe8;target=va+5+int.from_bytes(b[1:],'little',signed=True);assert target in starts or target==0x102de362
   fixes.append({'offset':va-a+1,'kind':'relative','target':hex(target)})
 header.append('inline constexpr unsigned char '+name+'ChainBytes[]={'+','.join(hex(v) for v in code)+'};')
 manifest.append({'name':name,'original_start':hex(a),'size':len(code),'relocations':fixes})
Path('work/engine_projection_inverse_native_bytes.h').write_text('\n'.join(header)+'\n')
result={'engine_sha256':sha,'routines':manifest,'constants':{hex(a):hex(b) for a,b in constants.items()},'security_cookie':'102de362 comparator/return instructions inspected; relocation requires matching owned cookie and trap on mismatch. Full original failure handler not included.','status':'Original full inverse bytes extracted and contiguous instruction coverage asserted; relocation dependencies closed except explicitly identified cookie-check branch. Execution not performed.','limitations':'Preparation only; no native inverse parity claim or live renderer integration.'}
Path('outputs/sfm64-engine-projection-inverse-native-preparation.json').write_text(json.dumps(result,indent=2));print('routines',len(manifest),'bytes',sum(r['size'] for r in manifest),'relocations',sum(len(r['relocations']) for r in manifest))
