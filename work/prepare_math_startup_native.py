from pathlib import Path
import json,struct
exec(Path('work/recover_command_buffer_creation.py').read_text().split('vertex=region')[0])
r=json.loads(Path('outputs/sfm64-math-library-startup-recovery.json').read_text());assert r['sha256']==sha
bodies=[r['regions']['initializer']]
opt=im.u32(60)+24;relocations=[];rva,size=struct.unpack_from('<II',im.b,opt+136);cursor=im.offset(im.base+rva);end=cursor+size
while cursor<end:
 page,length=struct.unpack_from('<II',im.b,cursor);assert length>=8 and cursor+length<=end
 for o in range(cursor+8,cursor+length,2):
  entry=im.u16(o);assert entry>>12 in (0,3)
  if entry>>12==3:relocations.append(im.base+page+(entry&4095))
 cursor+=length
assert cursor==end
header='#pragma once\n';metadata=[]
for i,body in enumerate(bodies):
 begin=int(body['start'],16);limit=int(body['end'],16);raw=im.b[im.offset(begin):im.offset(begin)+limit-begin];assert raw==b''.join(bytes.fromhex(x['bytes']) for x in body['instructions'])
 boundaries={int(x['address'],16) for x in body['instructions']};calls=[]
 for x in body['instructions']:
  a=int(x['address'],16);b=bytes.fromhex(x['bytes'])
  if b[0]==0xe8:
   target=a+5+struct.unpack_from('<i',b,1)[0];assert target in (0x100d5440,0x100d5020,0x100d5eb0);calls.append((a-begin,target))
  elif b[0]==0x0f and 0x80<=b[1]<=0x8f:assert a+6+struct.unpack_from('<i',b,2)[0] in boundaries
  elif b[0]==0xeb or 0x70<=b[0]<=0x7f:assert a+2+struct.unpack_from('<b',b,1)[0] in boundaries
 fixups=[(a-begin,im.word(a)) for a in relocations if begin<=a<limit]
 assert all(v in {0x10198ae0,0x100fa1fc,0x1015247c,0x100fa23c} for o,v in fixups)
 header+='static const unsigned char mathStartupBytes'+str(i)+'[]={'+','.join(map(str,raw))+'};\n'
 for name,entries in [('Fixups',fixups),('Calls',calls)]:header+='static const unsigned mathStartup'+name+str(i)+'[][2]={'+','.join('{'+str(o)+','+str(v)+'}' for o,v in (entries or [(0,0)]))+'};\n'
 metadata.append(dict(file_offset=im.offset(begin),size=len(raw),fixups=fixups,calls=calls))
header+='struct MathStartupBody{const unsigned char* bytes;unsigned size,fileOffset;const unsigned (*fixups)[2];unsigned fixupCount;const unsigned (*calls)[2];unsigned callCount;};\n'
header+='static const MathStartupBody mathStartupBodies[]={'+','.join('{mathStartupBytes'+str(i)+','+str(m['size'])+','+str(m['file_offset'])+',mathStartupFixups'+str(i)+','+str(len(m['fixups']))+',mathStartupCalls'+str(i)+','+str(len(m['calls']))+'}' for i,m in enumerate(metadata))+'};\n'
assert len(metadata)==1 and metadata[0]['size']==114
assert len(metadata[0]['calls'])==3
header+='static const char mathStartupDiagnostic[]='+json.dumps(im.string(0x1015247c))+';'+chr(10)
Path('work/math_startup_native_init.h').write_text(header)
Path('outputs/sfm64-math-startup-native-preparation.json').write_text(json.dumps(dict(sha256=sha,bodies=metadata,ownership='Original114byte math initializer; three controlled trigonometric/gamma/seed calls, restricted initialization flag/CPU getter/diagnostic string/import relocation bindings. Short conditional branches verified.',native_executed=False),indent=2)+'\n')
print('verified_math_startup_graph_bytes='+str(sum(m['size'] for m in metadata)))








