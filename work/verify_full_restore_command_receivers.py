from pathlib import Path
import json,re,hashlib
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest();assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
report=json.loads(Path('outputs/sfm64-full-restore-indirect-slot-audit.json').read_text());assert report['sha256']==sha
sites=[]
for s in report['sites']:
 if s.get('slot') not in (36,57,69):continue
 context=s['context']
 for row in context:
  a=int(row['address'],16);b=bytes.fromhex(row['bytes']);assert im.b[im.offset(a):im.offset(a)+len(b)]==b
 v=s['nearest_vtable_register_load'];assert len(v)==1
 m=re.fullmatch(r'mov\s+(eax|ecx|edx),dword ptr \[(eax|ebx|ecx|edx|esi|edi)\]',v[0]['instruction']);assert m
 receiver=m[2];before=context[:context.index(v[0])];origin=[]
 if s['slot']==36:
  assert receiver=='ebx';binding='full_owner';assert any(r['instruction']=='mov         ecx,ebx' for r in context[context.index(v[0]):])
 else:
  binding='device_at_owner648'
  for row in reversed(before):
   if re.match(r'mov\s+'+receiver+',',row['instruction']):origin=[row];break
  assert origin and re.fullmatch(r'mov\s+'+receiver+r',dword ptr \[ebx\+00000648h\]',origin[0]['instruction'])
  after=context[context.index(origin[0]):]
  assert not any(r['instruction'].startswith('call') for r in after[:-1])
  assert any(re.fullmatch(r'push\s+'+receiver,r['instruction']) for r in after)
 sites.append({'call':s['call'],'slot':s['slot'],'binding':binding,'receiver_load':origin or [{'register':'EBX','meaning':'entry owner'}],'vtable_load':v,'context':context})
assert len(sites)==34
out={'sha256':sha,'sites':sites,'counts':{'owner_slot36':18,'device_slot69':12,'device_slot57':4},'interpretation':'Correction: owner slot36 is Boolean gate100261a0 (ownerA8 nonzero OR flagsAC bit1 OR depth2CC nonzero); direct device methods reload owner648 before vtable load and push the same receiver. Queue-enable/capacity/cursor publication semantics are not inferred from slot alone.','limits':'Selected local receiver/load chains byte-checked, not whole-function execution or actual device behavior. Other31fixed-slot calls still require receiver binding.'}
Path('outputs/sfm64-full-restore-command-receivers.json').write_text(json.dumps(out,indent=2)+'\n');print('full_restore_command_receiver_sites=34 verified')

