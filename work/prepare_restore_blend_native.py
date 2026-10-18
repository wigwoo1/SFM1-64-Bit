from pathlib import Path
import hashlib,json
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest();assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
r=json.loads(Path('outputs/sfm64-restore-blend-helper-recovery.json').read_text());assert r['sha256']==sha
begin,end=0x1002f410,0x1002fa77;covered={};calls=[]
allowed={0x1000df50,0x10002da0,0x1002f2e0,0x1002ebb0}
for row in r['instructions']:
 a=int(row['address'],16);b=bytes.fromhex(row['bytes']);assert im.b[im.offset(a):im.offset(a)+len(b)]==b
 for i,v in enumerate(b):assert a+i not in covered;covered[a+i]=v
 if b[0]==0xe8:
  assert len(b)==5;target=a+5+int.from_bytes(b[1:],'little',signed=True);assert target in allowed
  calls.append((a-begin,target))
assert set(covered)==set(range(begin,end)) and len(calls)==32
raw=bytes(covered[a] for a in range(begin,end))
opt=im.u32(60)+24;rv,size=im.u32(opt+136),im.u32(opt+140);pos=im.offset(im.base+rv);limit=pos+size;fixups=[]
allowedGlobals={0x1016dd08,0x1019092c,0x1016dd0c,0x1016e20c}
while pos<limit:
 page,length=im.u32(pos),im.u32(pos+4);assert length>=8 and pos+length<=limit
 for at in range(pos+8,pos+length,2):
  entry=im.u16(at)
  if entry>>12==3:
   a=im.base+page+(entry&4095)
   if begin<=a<end:
    assert a+4<=end;target=im.word(a);assert target in allowedGlobals;fixups.append((a-begin,target))
 pos+=length
assert len(fixups)==5
header='#pragma once\nstatic const unsigned restoreBlendByteCount='+str(len(raw))+';\nstatic const unsigned restoreBlendFileOffset='+str(im.offset(begin))+';\n'
for name,entries in [('Calls',calls),('Fixups',fixups)]:header+='static const unsigned restoreBlend'+name+'[][2]={'+','.join('{'+str(o)+','+str(t)+'}' for o,t in entries)+'};\n'
Path('work/restore_blend_native_init.h').write_text(header)
Path('outputs/restore-blend-verified-bytes.bin').write_bytes(raw)
out={'sha256':sha,'begin':hex(begin),'end':hex(end),'bytes':len(raw),'calls':[{'offset':o,'target':hex(t)} for o,t in calls],'fixups':[{'offset':o,'target':hex(t)} for o,t in fixups],
 'bindings':{'0x1016dd08':'capability slot265 signed captured sampler count','0x1019092c':'slot49 lowbyte conditional sampler type8','0x1016dd0c':'slot36 lowbyte transition suppression','0x1016e20c':'slots9/10 shader invalidation; global reloaded separately'},
 'fixture_contract':'Writable receiver through sampler cache145+3*count; getter returns commandstate device0/queued10/cursor18/remaining1c. COM57 state and69 sampler submit. Bind nestedtransition1002f2e0/commit1002ebb0 explicitly, trap refill with adequate capacity.',
 'limits':'Preparation only; original execution comparison pending. Controlled nestedtransition is not fulloperation-handler coverage; invalid sampler count/state memory nativeparity not established.'}
Path('outputs/sfm64-restore-blend-native-preparation.json').write_text(json.dumps(out,indent=2)+'\n')
print(f'prepared_restore_blend_bytes={len(raw)} calls={len(calls)} absolute_fixups={len(fixups)}')
