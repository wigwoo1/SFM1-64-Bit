from pathlib import Path
import hashlib,json,struct
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest()
assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
sources=[('outputs/sfm64-transition-handler-table-recovery.json','handlers',0x1002ed50),('outputs/sfm64-transition-wrapper-helper-recovery.json','handlers',0x1002e510),('outputs/sfm64-restore-commit-setters-native-preparation.json','setters',0x1002e410),('outputs/sfm64-restore-commit-setters-native-preparation.json','setters',0x1002e490)]
opt=im.u32(60)+24;rv,size=im.u32(opt+136),im.u32(opt+140)
pos=im.offset(im.base+rv);limit=pos+size;relocations=[]
while pos<limit:
 page,length=im.u32(pos),im.u32(pos+4)
 assert length>=8 and length%2==0 and pos+length<=limit
 for at in range(pos+8,pos+length,2):
  entry=im.u16(at)
  if entry>>12==3:relocations.append(im.base+page+(entry&4095))
 pos+=length
assert pos==limit
header='#pragma once\n';reports=[]
expected_calls=[{0x1002e510},{0x1002e410,0x1002e490},{0x1000df50,0x10002da0},{0x1000df50,0x10002da0}]
for index,(path,key,begin) in enumerate(sources):
 source=json.loads(Path(path).read_text());assert source['sha256']==sha
 h=next(h for h in source[key] if int(h['begin'],16)==begin)
 end=int(h['end'],16);covered={};calls=[];branches=[]
 starts={int(row['address'],16) for row in h['instructions']}
 for row in h['instructions']:
  a=int(row['address'],16);b=bytes.fromhex(row['bytes'])
  assert im.b[im.offset(a):im.offset(a)+len(b)]==b
  for n,v in enumerate(b):
   assert a+n not in covered
   covered[a+n]=v
  if b[0]==0xe8:
   assert len(b)==5;target=a+5+struct.unpack('<i',b[1:])[0]
   assert target in expected_calls[index];calls.append((a-begin,target))
  if b[0]==0xeb or 0x70<=b[0]<=0x7f:
   target=a+2+struct.unpack('<b',b[1:2])[0];assert target in starts;branches.append((a,target))
  elif b[0]==0x0f and len(b)>=2 and 0x80<=b[1]<=0x8f:
   target=a+6+struct.unpack('<i',b[2:6])[0];assert target in starts;branches.append((a,target))
 assert set(covered)==set(range(begin,end))
 assert {target for _,target in calls}==expected_calls[index]
 fixups=[]
 for a in relocations:
  if begin<=a<end:
   assert a+4<=end;target=im.word(a)
   assert target==([0x10183fc4,0x1016dd08,None,None][index]);fixups.append((a-begin,target))
 assert len(fixups)==([1,1,0,0][index])
 raw=bytes(covered[a] for a in range(begin,end));Path(f'outputs/transition-depth-body-{index}.bin').write_bytes(raw)
 header+=f'static const unsigned transitionDepth{index}Size={len(raw)},transitionDepth{index}Offset={im.offset(begin)};\n'
 header+=f'static const unsigned transitionDepth{index}Calls[][2]={{'+','.join('{'+str(o)+','+str(t)+'}' for o,t in calls)+'};\n'
 if fixups:header+=f'static const unsigned transitionDepth{index}Fixups[][2]={{'+','.join('{'+str(o)+','+str(t)+'}' for o,t in fixups)+'};\n'
 reports.append({'index':index,'begin':hex(begin),'end':hex(end),'bytes':len(raw),'calls':calls,'fixups':fixups,'branches':branches})
Path('work/transition_depth_native_init.h').write_text(header)
out={'sha256':sha,'bodies':reports,'binding_plan':'Wrapper loads cache receiver once; actual depth helper calls actual cached setters with shared command receiver; backend slot261 callback can mutate state flags before mode publication. Refill trapped with sufficient storage.','limits':'Exact byte preparation only; combined original execution and x64 comparison pending.'}
Path('outputs/sfm64-transition-depth-native-preparation.json').write_text(json.dumps(out,indent=2)+'\n')
print(f'prepared_depth_chain_bytes={sum(h["bytes"] for h in reports)} calls={sum(len(h["calls"]) for h in reports)} global_fixups={sum(len(h["fixups"]) for h in reports)}')

