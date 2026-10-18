from pathlib import Path
import hashlib,json,re
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest();assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
begin,end=0x1002f2e0,0x1002f34b;rows=[];pending=None
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
 if m:
  if pending:rows.append(pending)
  pending={'address':int(m[1],16),'bytes':bytes.fromhex(m[2]),'instruction':m[3].strip()}
 elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):pending['bytes']+=bytes.fromhex(line)
if pending:rows.append(pending)
selected=[r for r in rows if begin<=r['address']<end];covered={};starts={r['address'] for r in selected}
for row in selected:
 a,b=row['address'],row['bytes'];assert im.b[im.offset(a):im.offset(a)+len(b)]==b
 for i,v in enumerate(b):assert a+i not in covered;covered[a+i]=v
 if row['instruction'].startswith('j'):assert int(row['instruction'].split()[1],16) in starts
 assert b[0]!=0xe8
assert set(covered)==set(range(begin,end))
raw=bytes(covered[a] for a in range(begin,end))
opt=im.u32(60)+24;rv,size=im.u32(opt+136),im.u32(opt+140);pos=im.offset(im.base+rv);limit=pos+size;fixups=[]
while pos<limit:
 page,length=im.u32(pos),im.u32(pos+4);assert length>=8 and pos+length<=limit
 for at in range(pos+8,pos+length,2):
  entry=im.u16(at)
  if entry>>12==3:
   a=im.base+page+(entry&4095)
   if begin<=a<end:
    assert a+4<=end;target=im.word(a);assert target in (0x1016e13c,0x1016e170);fixups.append((a-begin,target))
 pos+=length
assert len(fixups)==2
Path('outputs/transition-byte-dispatch-verified.bin').write_bytes(raw)
header='#pragma once\nstatic const unsigned transitionDispatchByteCount='+str(len(raw))+';\nstatic const unsigned transitionDispatchFileOffset='+str(im.offset(begin))+';\nstatic const unsigned transitionDispatchFixups[][2]={'+','.join('{'+str(o)+','+str(t)+'}' for o,t in fixups)+'};\n'
Path('work/transition_byte_dispatch_native_init.h').write_text(header)
Path('outputs/sfm64-transition-byte-dispatch-native-preparation.json').write_text(json.dumps({'sha256':sha,'begin':hex(begin),'end':hex(end),'bytes':len(raw),'fixups':fixups,'instructions':[{'address':hex(r['address']),'bytes':r['bytes'].hex(),'instruction':r['instruction']} for r in selected],'contract':'thiscall owner state-tableptr+58,operationptr+b8; stack signedkind,offset,count; state stride72; cdecltwoarg handler. Count<=0 skips pointer access. Both tableoperands remap controlled13ordinary/2packed. Captures state/operation pointers once, reloads nextbyte aftercallback.','limits':'Preparation only; nativeexecutioncomparison/actualhandler semantics pending. Validtableindices only; no native invalidopcode safety.'},indent=2)+'\n')
print(f'prepared_transition_dispatch_bytes={len(raw)} handler_table_fixups={len(fixups)}')
