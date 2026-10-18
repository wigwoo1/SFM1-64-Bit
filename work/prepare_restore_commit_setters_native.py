from pathlib import Path
import hashlib,json,re
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest();assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
rows=[];pending=None
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
 if m:
  if pending:rows.append(pending)
  pending={'address':int(m[1],16),'bytes':bytes.fromhex(m[2]),'instruction':m[3].strip()}
 elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):pending['bytes']+=bytes.fromhex(line)
if pending:rows.append(pending)
report=[];header='#pragma once\n'
for name,begin,end,cache,key in [('Depth',0x1002e410,0x1002e488,0x11c,7),('Compare',0x1002e490,0x1002e508,0x120,23)]:
 selected=[r for r in rows if begin<=r['address']<end];covered={};calls=[];starts={r['address'] for r in selected}
 for r in selected:
  a,b=r['address'],r['bytes'];assert im.b[im.offset(a):im.offset(a)+len(b)]==b
  for i,v in enumerate(b):assert a+i not in covered;covered[a+i]=v
  if b[0]==0xe8:
   target=a+5+int.from_bytes(b[1:],'little',signed=True);assert target in (0x1000df50,0x10002da0);calls.append((a-begin,target))
  if r['instruction'].startswith('j'):assert int(r['instruction'].split()[1],16) in starts
 assert set(covered)==set(range(begin,end)) and len(calls)==2
 raw=bytes(covered[a] for a in range(begin,end));Path('outputs/restore-commit-setter-'+name.lower()+'.bin').write_bytes(raw)
 header+=f'static const unsigned restoreCommit{name}Size={len(raw)},restoreCommit{name}Offset={im.offset(begin)};\n'
 header+=f'static const unsigned restoreCommit{name}Calls[][2]={{'+','.join('{'+str(o)+','+str(t)+'}' for o,t in calls)+'};\n'
 report.append({'name':name,'begin':hex(begin),'end':hex(end),'cache_offset':hex(cache),'state_key':key,'bytes':len(raw),'calls':calls,'instructions':[{'address':hex(r['address']),'bytes':r['bytes'].hex(),'instruction':r['instruction']} for r in selected]})
Path('work/restore_commit_setters_native_init.h').write_text(header)
Path('outputs/sfm64-restore-commit-setters-native-preparation.json').write_text(json.dumps({'sha256':sha,'setters':report,'policy':'Equal cached DWORD skips all; otherwise getter/refill or device57 state submission, then publish requested value to cache even after E_FAIL/callback mutation. No suppression gate in these bodies.','limits':'Exact bytes/calls/internalbranches checked; nativeexecution and fullnestedcommit integration pending.'},indent=2)+'\n')
print('prepared_commit_setters_bytes=240 setters=2')
