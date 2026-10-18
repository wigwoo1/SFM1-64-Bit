from pathlib import Path
import json,re,struct
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
r=json.loads(Path('outputs/sfm64-owner-reset-cleanup-recovery.json').read_text());rows=[x for x in r['instructions'] if 0x1000e550<=int(x['address'],16)<0x1000ea5f]
constants={hex(a):im.word(a) for a in [0x100fa5f8,0x100fc698]};stack=[];registers={'edi':0,'ebx':constants['0x100fa5f8']};steps=[]
for row in rows:
 a=int(row['address'],16);i=row['instruction']
 if a==0x1000e867:registers['eax']=constants['0x100fc698']
 if i.startswith('push'):
  op=i.split()[1]
  if op in registers:stack.append(registers[op])
  elif re.fullmatch(r'[0-9A-F]+h?',op):stack.append(int(op.rstrip('h'),16)&0xffffffff)
  else:stack.append(None)
 if i.startswith('call'):
  assert i.split()[1]=='1000E490';key=stack.pop();value=stack.pop();assert key is not None and value is not None
  predicate='always'
  if 0x1000e793<a<0x1000e7ab:predicate='capability byte+5a9 bit10 and vendor DWORD+4dc equals1002'
  elif 0x1000e7b2<a<0x1000e7c5:predicate='live capability byte+5a9 bit20'
  steps.append({'call_address':row['address'],'state':key,'value_bits':value,'predicate':predicate})
assert len(steps)==91
out={'verified_source_sha256':r['sha256'],'constant_bits':constants,'ordered_state_calls':steps,'limits':'Statically derived call argument sequence, branch predicates from verified instructions. No original initializer execution or x64 complete default-state policy yet.'}
Path('outputs/sfm64-default-render-state-sequence.json').write_text(json.dumps(out,indent=2)+'\n')
print('mapped_default_state_calls='+str(len(steps)));print('conditional_calls='+str(sum(s['predicate']!='always' for s in steps)))
