from pathlib import Path
import json,struct
exec(Path('work/recover_command_buffer_creation.py').read_text().split('vertex=region')[0])
r=json.loads(Path('outputs/sfm64-gamma-startup-recovery.json').read_text());assert r['sha256']==sha
body=r['body'];begin=int(body['start'],16);end=int(body['end'],16)
raw=im.b[im.offset(begin):im.offset(begin)+end-begin];assert raw==b''.join(bytes.fromhex(x['bytes']) for x in body['instructions'])
opt=im.u32(60)+24;rv,size=struct.unpack_from('<II',im.b,opt+136);cursor=im.offset(im.base+rv);limit=cursor+size;rel=[]
while cursor<limit:
 page,length=struct.unpack_from('<II',im.b,cursor);assert length>=8 and cursor+length<=limit
 for o in range(cursor+8,cursor+length,2):
  entry=im.u16(o);assert entry>>12 in (0,3)
  if entry>>12==3:rel.append(im.base+page+(entry&4095))
 cursor+=length
assert cursor==limit
fixups=[(a-begin,im.word(a)) for a in rel if begin<=a<end]
allowed={int(c['address'],16) for c in r['constants']}|{int(t['address'],16) for t in r['tables']}
assert all(v in allowed for _,v in fixups)
calls=[(int(c['address'],16)-begin,int(c['target'],16)) for c in body['direct_calls']]
h='#pragma once\nstatic const unsigned char gammaOriginalBytes[]={'+','.join(map(str,raw))+'};\n'
h+='static const unsigned gammaOriginalFileOffset='+str(im.offset(begin))+';\n'
for name,entries in [('Fixups',fixups),('Calls',calls)]:h+='static const unsigned gammaOriginal'+name+'[][2]={'+','.join('{'+str(o)+','+str(v)+'}' for o,v in entries)+'};\n'
h+='static uintptr_t gammaAddress(unsigned address,StudioGammaTablesCandidate& t){switch(address){\n'
for c in r['constants']:h+='case '+c['address']+':return reinterpret_cast<uintptr_t>(&studioGammaConstants.'+c['name']+');\n'
for t in r['tables']:h+='case '+t['address']+':return reinterpret_cast<uintptr_t>(t.'+t['name']+'.data());\n'
h+='default:std::abort();}}\n'
Path('work/gamma_startup_native_init.h').write_text(h)
Path('outputs/sfm64-gamma-startup-native-preparation.json').write_text(json.dumps(dict(sha256=sha,file_offset=im.offset(begin),bytes=len(raw),fixups=fixups,calls=calls,limits='Controlled float/double power providers. Not installed optimized power execution.'),indent=2)+'\n')
print('gamma_original_prepared_bytes=840 fixups='+str(len(fixups))+' calls=7')
