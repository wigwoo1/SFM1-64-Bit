"""Recover concrete indexed setter, scheduling and submission dependencies."""
from pathlib import Path
import hashlib,json,re
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest()
assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
assert im.word(0x100fcedc+206*4)==0x10026b70
ranges=[(0x10026b70,0x10026bc2),(0x10026890,0x100268f7),(0x1001f8a0,0x1001fb8f)]
rows=[];pending=None
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
 if m:
  address=int(m[1],16);pending=None
  if any(a<=address<b for a,b in ranges):
   pending={'address':address,'bytes':bytes.fromhex(m[2]),'instruction':m[3].strip()};rows.append(pending)
 elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):pending['bytes']+=bytes.fromhex(line)
covered=set()
for row in rows:
 a=row['address'];raw=row['bytes'];offset=im.offset(a)
 assert im.b[offset:offset+len(raw)]==raw
 covered.update(range(a,a+len(raw)))
assert covered=={a for begin,end in ranges for a in range(begin,end)}
report={'sha256':sha,'verified_regions':[{'begin':hex(a),'end':hex(b),'bytes':b-a} for a,b in ranges],
 'concrete_slot':{'table':'0x100fcedc','slot':206,'target':'0x10026b70'},
 'scheduler':'10026890 selects owner+2e8c+group*20 vector. Captures prior length, grows via1002ef20 if needed, increments length, refreshes mirror data pointer+10 and inserts callback at previous length. Group1 therefore uses owner+2ea0, matching the restore tail callback list.',
 'callback':'1001f8a0 reads external service1019092c slot31 count; nonpositive returns. Traverses current state at argument2+aa0 and previous state at argument3+aa8 in24-byte strides; force argument4 bypasses equality tests. Subsequent branches submit queue/direct commands and update cached fields.',
 'integration_requirement':'Widen callback pointers and vector storage consistently with restore dispatch; preserve group/ordering and owned buffer pointers across state snapshots.',
 'call_inventory':[{'address':hex(r['address']),'instruction':r['instruction']} for r in rows if r['instruction'].startswith('call')],
 'native_execution_verified':False,'limits':'Full three raw bodies verified, concrete slot identified. Callback field identities, external count service, vector growth/copy and GPU command semantics remain incomplete; no x64 ABI inferred from byte offsets.'}
Path('outputs/sfm64-restore-indexed-callback-chain.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'verified_chain_bytes={len(covered)} concrete_slot206=1 callback_group1_restore_tail_link=1')
