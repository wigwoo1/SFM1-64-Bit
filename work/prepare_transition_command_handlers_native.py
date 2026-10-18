from pathlib import Path
import hashlib,json,struct
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest()
assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
r=json.loads(Path('outputs/sfm64-transition-handler-table-recovery.json').read_text())
assert r['sha256']==sha
targets=[0x1002e010,0x1002e070,0x1002e0e0,0x1002e170,0x1002df70]
opt=im.u32(60)+24
rv,size=im.u32(opt+136),im.u32(opt+140)
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
for index,begin in enumerate(targets):
 h=next(h for h in r['handlers'] if int(h['begin'],16)==begin)
 end=int(h['end'],16);covered={};calls=[];branches=[]
 boundaries={int(row['address'],16) for row in h['instructions']}
 for row in h['instructions']:
  a=int(row['address'],16);b=bytes.fromhex(row['bytes'])
  assert im.b[im.offset(a):im.offset(a)+len(b)]==b
  for n,v in enumerate(b):
   assert a+n not in covered
   covered[a+n]=v
  if b[0]==0xe8:
   assert len(b)==5
   target=a+5+struct.unpack('<i',b[1:])[0]
   assert target in (0x1000df50,0x10002da0)
   calls.append((a-begin,target))
  elif b[0]==0xeb or 0x70<=b[0]<=0x7f:
   target=a+2+struct.unpack('<b',b[1:2])[0]
   assert target in boundaries
   branches.append((a,target))
  elif b[:1]==b'\x0f' and len(b)>=2 and 0x80<=b[1]<=0x8f:
   target=a+6+struct.unpack('<i',b[2:6])[0]
   assert target in boundaries
   branches.append((a,target))
 assert set(covered)==set(range(begin,end))
 assert len(calls)==2 and {t for _,t in calls}=={0x1000df50,0x10002da0}
 raw=bytes(covered[a] for a in range(begin,end));fixups=[]
 for a in relocations:
  if begin<=a<end:
   assert a+4<=end
   target=im.word(a)
   assert target in (0x10183fc4,0x1019092c)
   fixups.append((a-begin,target))
 assert len(fixups)==([0,0,2,0,1][index])
 Path(f'outputs/transition-command-handler-{index}.bin').write_bytes(raw)
 header+=f'static const unsigned transitionCommand{index}Size={len(raw)},transitionCommand{index}Offset={im.offset(begin)};\n'
 header+=f'static const unsigned transitionCommand{index}Calls[][2]={{'+','.join('{'+str(o)+','+str(t)+'}' for o,t in calls)+'};\n'
 if fixups:header+=f'static const unsigned transitionCommand{index}Fixups[][2]={{'+','.join('{'+str(o)+','+str(t)+'}' for o,t in fixups)+'};\n'
 reports.append({'index':index,'begin':hex(begin),'end':hex(end),'bytes':len(raw),'calls':calls,'fixups':fixups,'internal_branches':branches})
Path('work/transition_command_handlers_native_init.h').write_text(header)
out={'sha256':sha,'handlers':reports,'bindings':{'0x1000df50':'command receiver getter','0x10002da0':'queue refill thiscall','0x10183fc4':'live cache receiver pointer cell, +0x140 publication','0x1019092c':'capability object pointer cell, slot49 lowbyte gate'},'limits':'Preparation only. Native execution, refill semantics, original queue consumer and complete application are unverified.'}
Path('outputs/sfm64-transition-command-native-preparation.json').write_text(json.dumps(out,indent=2)+'\n')
print(f'prepared_command_handler_bytes={sum(h["bytes"] for h in reports)} direct_calls={sum(len(h["calls"]) for h in reports)} global_fixups={sum(len(h["fixups"]) for h in reports)}')
