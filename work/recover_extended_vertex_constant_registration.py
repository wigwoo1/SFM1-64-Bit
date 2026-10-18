"""Verify capability gate tying the extended wrapper to allocated capacity."""
from pathlib import Path
import hashlib,json,re
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest()
assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
begin,end=0x10026fb0,0x1002703f;rows=[];pending=None
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
 if m:
  a=int(m[1],16);pending=None
  if begin<=a<end:pending=[a,bytes.fromhex(m[2]),m[3].strip()];rows.append(pending)
 elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):pending[1]+=bytes.fromhex(line)
covered={}
for a,raw,_ in rows:
 offset=im.offset(a);assert im.b[offset:offset+len(raw)]==raw
 for i,b in enumerate(raw):covered[a+i]=b
assert set(covered)==set(range(begin,end))
raw=bytes(covered[a] for a in range(begin,end))
assert raw[8:18]==bytes.fromhex('81b83405000000060000')
assert im.string(0x100fd424)
slots=[n for n in range(220) if im.word(0x100fcedc+n*4)==begin]
report={'sha256':sha,'body':{'begin':hex(begin),'end':hex(end),'bytes':len(raw)},'concrete_table_slots':slots,
 'capability_gate':'Load global1016d130; signed count at+534 must be >=600hex/1536. Below threshold returns before reading input data or registration.',
 'allocation_link':'Existing recovered allocation map uses capability+534 for vertexfloat primary owner19e0 and staging2858, stride16. Subobject+2588 aliases staging owner2858. Wrapper indexes1024..1535.',
 'input_flow':'Signed wrapping start+count >512 warns and clips count to512-start; count<=0 skips. Copies count*16 input bytes into staging[(start+1024)*4]. Native input start has no explicit nonnegative check.',
 'registration':'When subobject byte2bb8 bit1 clear, schedule callback10011610 in category0 through10026890 then OR livebyte bit1. Subsequent copies still occur when callback already scheduled.',
 'warning':im.string(0x100fd424),'calls':[{'address':hex(a),'instruction':text} for a,_,text in rows if text.startswith('call')],
 'limits':'Full143-byte path statically verified, not executed. Capability mutations, invalid negative indices, actual device register support and full lifecycle remain unresolved. Gate explains allocation domain but does not prove GPU acceptance.'}
Path('outputs/sfm64-extended-vertex-constant-registration.json').write_text(json.dumps(report,indent=2)+'\n')
Path('work/extended_vertex_constant_registration_native_init.h').write_text('#pragma once\nstatic const unsigned char extendedVertexRegistrationBytes[]={'+','.join(map(str,raw))+'};\n')
print(f'verified_registration_bytes={len(raw)} capacity_gate=1536 concrete_slots={slots}')
