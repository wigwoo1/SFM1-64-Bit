from pathlib import Path
import json,struct,hashlib
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tier0.dll');r=json.loads(Path('outputs/sfm64-profile-counter-service-recovery.json').read_text());assert hashlib.sha256(im.b).hexdigest()==r['tier0_sha256']
opt=im.u32(60)+24;rva,size=struct.unpack_from('<II',im.b,opt+136);cursor=im.offset(im.base+rva);end=cursor+size;relocations=[]
while cursor<end:
 page,length=struct.unpack_from('<II',im.b,cursor);assert length>=8 and length%2==0 and cursor+length<=end
 for pos in range(cursor+8,cursor+length,2):
  entry=im.u16(pos);assert entry>>12 in (0,3)
  if entry>>12==3:relocations.append(im.base+page+(entry&4095))
 cursor+=length
assert cursor==end
specs=[('factory',0x10013e80,0x10013f7a,r['verified_creation']),('reset',0x10013f80,0x10013fc4,r['verified_group_reset']),('cleanup',0x10014690,0x1001474f,r['verified_profile_cleanup'])]
header='#pragma once\n';prepared=[]
for index,(name,start,limit,rows) in enumerate(specs):
 raw=im.b[im.offset(start):im.offset(start)+limit-start];coverage={};calls=[];boundaries={int(row['address'],16) for row in rows};branches=[]
 for row in rows:
  a=int(row['address'],16);b=bytes.fromhex(row['bytes']);assert im.b[im.offset(a):im.offset(a)+len(b)]==b
  for n,v in enumerate(b):assert a+n not in coverage;coverage[a+n]=v
  if b[0]==0xe8:target=a+5+struct.unpack_from('<i',b,1)[0];assert target in {0x1001b1b1,0x1001e482,0x1001e4d9,0x1001b026,0x10014630};calls.append((a-start,target))
  if b[0]==0xeb or 0x70<=b[0]<=0x7f:target=a+2+struct.unpack_from('<b',b,1)[0];assert target in boundaries;branches.append((a,target))
  elif b[0]==0x0f and 0x80<=b[1]<=0x8f:target=a+6+struct.unpack_from('<i',b,2)[0];assert target in boundaries;branches.append((a,target))
 assert raw==bytes(coverage[a] for a in range(start,limit))
 fixes=[(a-start,im.word(a)) for a in relocations if start<=a<limit]
 assert {v for a,v in fixes}==({0x1002f040,0x1007a380} if index==0 else set())
 header+=f'static const unsigned char profileCounterBytes{index}[]={{'+','.join(map(str,raw))+'};\n'
 for label,entries in [('Calls',calls),('Fixups',fixes)]:
  header+=f'static const unsigned profileCounter{label}{index}[][2]={{'+(','.join('{'+str(a)+','+str(v)+'}' for a,v in entries) if entries else '{0,0}')+'};\n'
 prepared.append(dict(name=name,size=len(raw),file_offset=im.offset(start),calls=calls,fixups=fixes,branches=branches))
header+='struct ProfileCounterNativeBody{const unsigned char* bytes;unsigned size,fileOffset;const unsigned (*calls)[2];unsigned callCount;const unsigned (*fixups)[2];unsigned fixupCount;};\n'
header+='static const ProfileCounterNativeBody profileCounterBodies[]={'+','.join('{profileCounterBytes'+str(i)+','+str(m['size'])+','+str(m['file_offset'])+',profileCounterCalls'+str(i)+','+str(len(m['calls']))+',profileCounterFixups'+str(i)+','+str(len(m['fixups']))+'}' for i,m in enumerate(prepared))+'};\n'
Path('work/profile_counter_native_init.h').write_text(header)
Path('outputs/sfm64-profile-counter-native-preparation.json').write_text(json.dumps(dict(tier0_sha256=r['tier0_sha256'],methods=prepared,native_executed=False,contract='Remap factory thread-ID import, fallback counter, strcmp and allocation. Cleanup array free/name release/tree cleanup require explicit controls; root children and auxiliary array can be zero in bounded counter-only fixture. Profile storage must cover19cc and allcounter/name/group arrays. Returned counter identity must be checked before address normalization. Valid count0..255 only until signed overflow behavior separately recovered.',limits='Preparation only, not original execution or production profile lifecycle.'),indent=2)+'\n')
print('profile_counter_prepared_bytes=509 direct_calls='+str(sum(len(m['calls']) for m in prepared))+' absolute_fixups='+str(sum(len(m['fixups']) for m in prepared)))
