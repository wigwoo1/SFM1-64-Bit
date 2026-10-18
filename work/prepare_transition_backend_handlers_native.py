from pathlib import Path
import hashlib,json
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest();assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
r=json.loads(Path('outputs/sfm64-transition-handler-table-recovery.json').read_text());assert r['sha256']==sha
targets=[0x1002eac0,0x1002eb00,0x1002eb30,0x1002eb60]
opt=im.u32(60)+24;rv,size=im.u32(opt+136),im.u32(opt+140);pos=im.offset(im.base+rv);limit=pos+size;allFixups=[]
while pos<limit:
 page,length=im.u32(pos),im.u32(pos+4);assert length>=8 and pos+length<=limit
 for at in range(pos+8,pos+length,2):
  entry=im.u16(at)
  if entry>>12==3:allFixups.append(im.base+page+(entry&4095))
 pos+=length
headers='#pragma once\n';reports=[]
for index,begin in enumerate(targets):
 h=next(h for h in r['handlers'] if int(h['begin'],16)==begin);end=int(h['end'],16);covered={}
 for row in h['instructions']:
  a=int(row['address'],16);b=bytes.fromhex(row['bytes']);assert im.b[im.offset(a):im.offset(a)+len(b)]==b
  for i,v in enumerate(b):assert a+i not in covered;covered[a+i]=v
  assert b[0]!=0xe8,'Unexpected directcall'
 assert set(covered)==set(range(begin,end));raw=bytes(covered[a] for a in range(begin,end));fixups=[]
 for a in allFixups:
  if begin<=a<end:
   assert a+4<=end;target=im.word(a);assert target in (0x1019092c,0x1016dd08);fixups.append((a-begin,target))
 assert len(fixups)==(2 if index==0 else 1)
 Path(f'outputs/transition-backend-handler-{index}.bin').write_bytes(raw)
 headers+=f'static const unsigned transitionBackend{index}Size={len(raw)},transitionBackend{index}Offset={im.offset(begin)};\n'
 headers+=f'static const unsigned transitionBackend{index}Fixups[][2]={{'+','.join('{'+str(o)+','+str(t)+'}' for o,t in fixups)+'};\n'
 reports.append({'index':index,'begin':hex(begin),'end':hex(end),'bytes':len(raw),'fixups':fixups})
Path('work/transition_backend_handlers_native_init.h').write_text(headers)
out={'sha256':sha,'handlers':reports,'bindings':{'0x1019092c':'slot1 signed stagecount captured before state access','0x1016dd08':'stage slot262 reload eachloop, flag slots263/272 andcombined264'},'limits':'Exactbytes/relocations prepared only; nativeexecution andx64comparison pending. Controlled backendcallbacks do not prove fullbackend implementation.'}
Path('outputs/sfm64-transition-backend-native-preparation.json').write_text(json.dumps(out,indent=2)+'\n')
print(f'prepared_backend_handler_bytes={sum(h["bytes"] for h in reports)} global_fixups={sum(len(h["fixups"]) for h in reports)}')
