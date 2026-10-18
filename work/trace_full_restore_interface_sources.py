from pathlib import Path
import json,re,hashlib
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest();assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
audit=json.loads(Path('outputs/sfm64-full-restore-indirect-slot-audit.json').read_text())
source=json.loads(Path('outputs/sfm64-owner-reset-cleanup-recovery.json').read_text());assert audit['sha256']==source['sha256']==sha
rows=[r for r in source['instructions'] if 0x1001fb90<=int(r['address'],16)<0x100211e7]
for r in rows:
 a=int(r['address'],16);b=bytes.fromhex(r['bytes']);assert im.b[im.offset(a):im.offset(a)+len(b)]==b
def origin(register,before,depth=0):
 if register=='ebx':return {'anchor':'fullowner EBX from entry ECX'}
 if depth>8:return {'unresolved':'depth limit'}
 for r in reversed([r for r in rows if int(r['address'],16)<before]):
  m=re.fullmatch(r'(mov|lea)\s+'+register+r',(.+)',r['instruction'])
  if not m:continue
  out={'assignment':r};operand=m[2]
  if operand in ('eax','ebx','ecx','edx','esi','edi'):out['source']=origin(operand,int(r['address'],16),depth+1)
  stack=re.fullmatch(r'dword ptr \[ebp-([0-9A-F]+)h\]',operand)
  if stack:
   for prior in reversed([p for p in rows if int(p['address'],16)<int(r['address'],16)]):
    store=re.fullmatch(r'mov\s+dword ptr \[ebp-'+stack[1]+r'h\],(eax|ebx|ecx|edx|esi|edi)',prior['instruction'])
    if store:out['stack_store']=prior;out['source']=origin(store[1],int(prior['address'],16),depth+1);break
  return out
 return {'unresolved':'no preceding assignment'}
sites=[]
for s in audit['sites']:
 if s.get('slot') in (None,36,57,69):continue
 v=s['nearest_vtable_register_load'];assert len(v)==1
 m=re.fullmatch(r'mov\s+(eax|ecx|edx),dword ptr \[(eax|ebx|ecx|edx|esi|edi)(?:\+([0-9A-F]+)h)?\]',v[0]['instruction']);assert m
 sites.append({'call':s['call'],'slot':s['slot'],'vtable_load':v[0],'receiver_register_or_embedded_base':m[2],'embedded_vtable_displacement':m[3],'linear_origin':origin(m[2],int(v[0]['address'],16)),'context':s['context']})
assert len(sites)==31
Path('outputs/sfm64-full-restore-interface-source-traces.json').write_text(json.dumps({'sha256':sha,'sites':sites,'limits':'Byte-verified nearest linear provenance, not branch dominance. Caller-saved registers and values across callbacks need explicit control-flow validation before binding. No original fullfunction execution claimed.'},indent=2)+'\n')
print('full_restore_remaining_interface_sources=31 traced')
