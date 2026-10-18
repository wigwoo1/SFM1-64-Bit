from pathlib import Path
import re,json,hashlib
from collections import Counter
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest();assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
source=json.loads(Path('outputs/sfm64-owner-reset-cleanup-recovery.json').read_text());assert source['sha256']==sha
rows=[r for r in source['instructions'] if 0x1001fb90<=int(r['address'],16)<0x100211e7]
cursor=0x1001fb90
for row in rows:
 a=int(row['address'],16);b=bytes.fromhex(row['bytes']);assert a==cursor and im.b[im.offset(a):im.offset(a)+len(b)]==b;cursor+=len(b)
assert cursor==0x100211e7
sites=[];unresolved=[]
for index,row in enumerate(rows):
 if not row['instruction'].startswith('call') or bytes.fromhex(row['bytes'])[0]!=0xff:continue
 register=re.fullmatch(r'call\s+(eax|ecx|edx)',row['instruction'])
 context=rows[max(0,index-40):index+1]
 if not register:
  assert bytes.fromhex(row['bytes'])==bytes.fromhex('ff15c0a10f10')
  sites.append({'call':row['address'],'kind':'import','iat':'0x100fa1c0','context':context});continue
 reg=register[1];load=None
 for previous in reversed(context[:-1]):
  m=re.match(r'mov\s+'+reg+r',(.+)',previous['instruction'])
  if m:load=(previous,m[1]);break
 if not load:unresolved.append(row);continue
 if load[1]=='dword ptr [ecx+edi*4]':
  assert row['address'] in ('0x100210d0','0x10021130') and load[0]['bytes']=='8b14b9'
  sites.append({'call':row['address'],'kind':'indexed_callback_array','target_load':load[0],'index':'EDI','array_base':'ECX','context':context});continue
 m=re.fullmatch(r'dword ptr \[(eax|ecx|edx)(?:\+([0-9A-F]+)(h?))?\]',load[1])
 if not m:unresolved.append({'call':row,'nearest_load':load[0]});continue
 displacement=int(m[2],16 if m[3] else 10) if m[2] else 0;assert displacement%4==0
 prior=context[:context.index(load[0])]
 vtable=[]
 for r in reversed(prior):
  if re.match(r'mov\s+'+m[1]+',',r['instruction']):vtable=[r];break
 sites.append({'call':row['address'],'kind':'vtable_candidate','target_load':load[0],'vtable_byte_offset':displacement,'slot':displacement//4,'nearest_vtable_register_load':vtable,'context':context})
assert len(sites)+len(unresolved)==68
report={'sha256':sha,'span_bytes':5719,'sites':sites,'unresolved':unresolved,'slot_counts':dict(Counter(str(s['slot']) for s in sites if 'slot' in s)),'limits':'All original span bytes and target-load slot arithmetic checked. Nearest linear assignments are navigation evidence, not control-flow dominance or receiver identity proof. Fullowner execution and actual interfaces remain unverified.'}
Path('outputs/sfm64-full-restore-indirect-slot-audit.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'full_restore_indirect_sites={len(sites)} unresolved={len(unresolved)}')
