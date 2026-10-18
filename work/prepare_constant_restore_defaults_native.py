from pathlib import Path
import hashlib,json,struct
from pe_image import Image

image=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
report=json.loads(Path('outputs/sfm64-constant-restore-tail.json').read_text())
assert hashlib.sha256(image.b).hexdigest()==report['dll_sha256']
start,end=0x100156a0,0x10015bc4
raw=image.b[image.offset(start):image.offset(start)+end-start]
covered={};calls=[];indirect=[];starts=set()
for row in report['instructions']:
 address=int(row['address'],16);starts.add(address)
 covered.update({address+i:value for i,value in enumerate(bytes.fromhex(row['bytes']))})
 if row['instruction'].startswith('call'):
  operand=row['instruction'].split()[1]
  if len(operand)==8:
   target=int(operand,16);offset=address-start
   assert target in (0x10002da0,0x100df5ad)
   assert raw[offset]==0xe8 and address+5+struct.unpack_from('<i',raw,offset+1)[0]==target
   calls.append(dict(offset=offset,target=target))
  else:indirect.append(row)
assert raw==bytes(covered[a] for a in range(start,end))
branches=[]
for row in report['instructions']:
 if row['instruction'].startswith('j'):
  target=int(row['instruction'].split()[1],16);assert target in starts;branches.append(row)
opt=image.u32(60)+24;rva,size=struct.unpack_from('<II',image.b,opt+136)
cursor=image.offset(image.base+rva);limit=cursor+size;fixups=[]
allowed={int(a,16) for a in report['constants']}|{0x1017c370}
while cursor<limit:
 page,length=struct.unpack_from('<II',image.b,cursor);assert length>=8 and cursor+length<=limit
 for position in range(cursor+8,cursor+length,2):
  entry=image.u16(position);address=image.base+page+(entry&4095)
  if start<=address<end and entry>>12:
   assert entry>>12==3 and address+4<=end
   target=image.word(address);assert target in allowed,hex(target)
   fixups.append(dict(offset=address-start,target=target))
 cursor+=length
assert len(calls)==5 and len(indirect)==4
assert {f['target'] for f in fixups}==allowed
header='#pragma once\nstatic const unsigned char restoreDefaultsBytes[]={'+','.join(map(str,raw))+'};\n'
header+='static const unsigned restoreDefaultsFileOffset='+str(image.offset(start))+';\n'
for name,entries in [('Calls',calls),('Fixups',fixups)]:
 header+='static const unsigned restoreDefaults'+name+'[][2]={'+','.join('{'+str(e['offset'])+','+str(e['target'])+'}' for e in entries)+'};\n'
header+='static const unsigned restoreDefaultsConstants[][2]={'+','.join('{'+str(int(a,16))+','+str(int(v['bits'],16))+'}' for a,v in report['constants'].items())+'};\n'
Path('work/constant_restore_defaults_native_init.h').write_text(header)
out=dict(dll_sha256=report['dll_sha256'],bytes=len(raw),calls=calls,fixups=fixups,indirect_calls=indirect,branches=branches,native_executed=False,limits='Private oracle must remap constants/security cookie, bind device94 and queue state; four refill calls and final cookie checker require controlled callbacks. No native execution confirmed.')
Path('outputs/sfm64-constant-restore-defaults-native-preparation.json').write_text(json.dumps(out,indent=2)+'\n')
print(f'restore_defaults_prepared_bytes={len(raw)} fixups={len(fixups)} calls={len(calls)} indirect={len(indirect)} branches={len(branches)}')
