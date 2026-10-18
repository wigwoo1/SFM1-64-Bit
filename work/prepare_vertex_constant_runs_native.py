"""Verified original batching helper and restore wrapper for isolated oracle."""
from pathlib import Path
import hashlib,json,re,struct
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest()
assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
regions=[('runs',0x1000f1b0,0x1000f3d0),('wrapper',0x10011610,0x10011636)]
rows=[];pending=None
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
 if m:
  a=int(m[1],16);pending=None
  if any(begin<=a<end for _,begin,end in regions):pending=[a,bytes.fromhex(m[2]),m[3].strip()];rows.append(pending)
 elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):pending[1]+=bytes.fromhex(line)
coverage=set()
for a,raw,_ in rows:
 off=im.offset(a);assert im.b[off:off+len(raw)]==raw;coverage.update(range(a,a+len(raw)))
assert coverage=={a for _,begin,end in regions for a in range(begin,end)}
opt=im.u32(60)+24;cursor=im.offset(im.base+im.u32(opt+136));limit=cursor+im.u32(opt+140);fixups=[]
while cursor<limit:
 page,size=struct.unpack_from('<II',im.b,cursor);assert size>=8 and cursor+size<=limit
 for off in range(cursor+8,cursor+size,2):
  entry=im.u16(off);a=im.base+page+(entry&4095)
  if entry>>12==3 and any(begin<=a<end for _,begin,end in regions):fixups.append(a)
 cursor+=size
assert not fixups
header='#pragma once\n';reports=[]
for name,begin,end in regions:
 raw=im.b[im.offset(begin):im.offset(begin)+end-begin];calls=[];indirect=[]
 for a,prefix,text in rows:
  if not begin<=a<end or not text.startswith('call'):continue
  if prefix[0]==0xe8:
   target=a+5+struct.unpack('<i',prefix[1:])[0];assert target in {0x10002da0,0x100df030,0x1000f1b0};calls.append((a-begin,target))
  else:indirect.append(hex(a))
 assert len(calls)==(6 if name=='runs' else 1)
 assert len(indirect)==(2 if name=='runs' else 0)
 header+=f'static const unsigned char vertexConstant{name.title()}Bytes[]={{'+','.join(map(str,raw))+'};\n'
 header+=f'static const unsigned vertexConstant{name.title()}FileOffset={im.offset(begin)};\n'
 header+=f'static const unsigned vertexConstant{name.title()}Calls[][2]={{'+','.join('{'+str(a)+','+str(b)+'}' for a,b in calls)+'};\n'
 reports.append({'name':name,'begin':hex(begin),'bytes':len(raw),'calls':calls,'indirect_calls':indirect})
Path('work/vertex_constant_runs_native_init.h').write_text(header)
Path('outputs/sfm64-vertex-constant-runs-native-preparation.json').write_text(json.dumps({'sha256':sha,'regions':reports,'absolute_fixups':fixups,
 'bindings':'Helper receives command state in EAX and five stack arguments; wrapper is cdecl four arguments, supplies register start1024/count512. Remap refill and copy; private device COM94.',
 'scope':'Allocate arrays for full requested register domain; compare float-bit cache/payloads, command streams and run ordering including signed zeros, NaNs, force lowbyte and live pointer replacement.',
 'limits':'Preparation only; no original execution. Wrapper register-domain support, actual owner allocations, GPU/refill and whole restore remain unverified.'},indent=2)+'\n')
print(f'prepared_constant_batch_bytes={len(coverage)} absolute_fixups=0')
