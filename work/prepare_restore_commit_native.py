from pathlib import Path
import hashlib,json
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest();assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
r=json.loads(Path('outputs/sfm64-restore-commit-helper-recovery.json').read_text());assert r['sha256']==sha
begin,end=0x1002ebb0,0x1002ed30;covered={};calls=[]
allowed={0x1002e490,0x1002e410,0x1000df50,0x10002da0}
for row in r['instructions']:
 a=int(row['address'],16);b=bytes.fromhex(row['bytes']);assert im.b[im.offset(a):im.offset(a)+len(b)]==b
 for i,value in enumerate(b):assert a+i not in covered;covered[a+i]=value
 if b[0]==0xe8:
  assert len(b)==5;target=a+5+int.from_bytes(b[1:],'little',signed=True);assert target in allowed
  calls.append((a-begin,target))
assert set(covered)==set(range(begin,end)) and len(calls)==8
raw=bytes(covered[a] for a in range(begin,end))
p=im.u32(60);opt=p+24;rv,size=im.u32(opt+136),im.u32(opt+140);pos=im.offset(im.base+rv);limit=pos+size;fixups=[];scanned=0
while pos<limit:
 page,length=im.u32(pos),im.u32(pos+4);assert length>=8 and pos+length<=limit
 for entrypos in range(pos+8,pos+length,2):
  entry=im.u16(entrypos)
  if entry>>12==3:
   scanned+=1;a=im.base+page+(entry&4095)
   if begin<=a<end:fixups.append(a)
 pos+=length
assert not fixups
Path('outputs/restore-commit-verified-bytes.bin').write_bytes(raw)
header='#pragma once\nstatic const unsigned restoreCommitByteCount='+str(len(raw))+';\nstatic const unsigned restoreCommitFileOffset='+str(im.offset(begin))+';\n'
header+='static const unsigned restoreCommitCalls[][2]={'+','.join('{'+str(offset)+','+str(target)+'}' for offset,target in calls)+'};\n'
Path('work/restore_commit_native_init.h').write_text(header)
out={'sha256':sha,'begin':hex(begin),'end':hex(end),'bytes':len(raw),'calls':[{'offset':o,'target':hex(t)} for o,t in calls],'highlow_scanned':scanned,'fixups':fixups,
 'fixture_contract':'thiscall receiver writable through144. Remap1002e490 apply134(3),1002e410 apply135(1),1000df50 returning live command-state,10002da0 refill controlled/trapped. Command-state: device0,queued10,cursor18,remaining1c; device57 SetRenderState. Observe cache140 published beforegetter/submission. Both returnpaths are contained.',
 'limits':'Exact preparation only; nativeexecution/x64 comparison pending. Remapping nestedhelpers does not establish their implementations or actualGPU/refillconsumer.'}
Path('outputs/sfm64-restore-commit-native-preparation.json').write_text(json.dumps(out,indent=2)+'\n')
print(f'prepared_restore_commit_bytes={len(raw)} calls={len(calls)} highlow={scanned} fixups=0')
