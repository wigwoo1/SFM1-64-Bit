from pathlib import Path
import hashlib,json,re
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest();assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
source=json.loads(Path('outputs/sfm64-owner-reset-cleanup-recovery.json').read_text());assert source['sha256']==sha
rows=[r for r in source['instructions'] if 0x1001fb90<=int(r['address'],16)<0x100211e7]
by={int(r['address'],16):r for r in rows};succ={};branches=[];cursor=0x1001fb90
for a,r in by.items():
 b=bytes.fromhex(r['bytes']);assert a==cursor and im.b[im.offset(a):im.offset(a)+len(b)]==b;cursor=a+len(b);op=r['instruction'].split()[0]
 if op.startswith('ret'):succ[a]=[];continue
 if op.startswith('j'):
  if b[0] in (0xeb,0xe9):operand=b[1:]
  elif 0x70<=b[0]<=0x7f:operand=b[1:]
  else:assert b[0]==0x0f and 0x80<=b[1]<=0x8f;operand=b[2:]
  target=cursor+int.from_bytes(operand,'little',signed=True);assert target in by
  succ[a]=[target] if op=='jmp' else [cursor,target];branches.append({'address':hex(a),'target':hex(target)})
 else:succ[a]=[cursor]
 assert all(n in by for n in succ[a])
assert cursor==0x100211e7
entry=0x1001fb90;reachable=set();pending=[entry]
while pending:
 a=pending.pop()
 if a in reachable:continue
 reachable.add(a);pending.extend(succ[a])
pred={a:[] for a in reachable}
for a in reachable:
 for b in succ[a]:pred[b].append(a)
dom={a:({entry} if a==entry else set(reachable)) for a in reachable}
changed=True
while changed:
 changed=False
 for a in sorted(reachable):
  if a==entry:continue
  new={a}|set.intersection(*(dom[p] for p in pred[a]))
  if new!=dom[a]:dom[a]=new;changed=True
stores=[a for a,r in by.items() if re.match(r'mov\s+dword ptr \[ebp-40h\],',r['instruction'])]
assert stores==[0x1001fbbc] and by[stores[0]]['bytes']=='8975c0'
assert by[0x1001fbb0]['bytes']=='8db3d0020000' and 0x1001fbb0 in dom[stores[0]]
between=[r for a,r in by.items() if 0x1001fbb0<a<stores[0]]
assert not any(re.match(r'(mov|lea|pop|xor|add|sub)\s+esi(?:,|$)',r['instruction']) for r in between)
loads=[a for a,r in by.items() if re.fullmatch(r'mov\s+(eax|ecx|edx|esi|edi),dword ptr \[ebp-40h\]',r['instruction'])]
assert loads and all(stores[0] in dom[a] for a in loads)
report={'sha256':sha,'instruction_nodes':len(by),'reachable_nodes':len(reachable),'branches':branches,'saved_receiver_store':hex(stores[0]),'source':'LEA ESI,[EBX+0x2d0] at1001fbb0 dominates unique saved local store; ESI unchanged before store.','loads':[{'address':hex(a),'instruction':by[a]['instruction'],'store_dominates':True} for a in loads],
 'limits':'Intraprocedural normal control flow, calls modeled returning to next instruction. Explicit compiler stack-local stores checked; arbitrary callback stack corruption, exceptions and actual interface behavior not modeled. Does not prove receiver register remains unchanged from each load to each virtual call.'}
Path('outputs/sfm64-full-restore-saved-receiver-cfg.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'full_restore_cfg_nodes={len(by)} saved_receiver_loads={len(loads)} dominance_verified')
