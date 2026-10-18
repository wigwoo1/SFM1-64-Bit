"""Distinguish forced state publication from rendering readiness."""
from pathlib import Path
import hashlib,json,re
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest()
assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
assert 0x2d0+0x2e08==0x30d8
begin,end=0x10026b70,0x10026bc2
covered={};rows=[];address=None
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
 if m:
  address=int(m[1],16);raw=bytes.fromhex(m[2])
  if begin<=address<end or '00002E08h' in m[3]:
   offset=im.offset(address);assert im.b[offset:offset+len(raw)]==raw
   rows.append({'address':hex(address),'instruction':m[3].strip(),'bytes':raw.hex()})
 elif address is not None and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):raw=bytes.fromhex(line)
 else:continue
 for byte in raw:
  if begin<=address<end:covered[address]=byte
  address+=1
assert len(covered)==end-begin
body=bytes(covered[a] for a in range(begin,end))
assert body==im.b[im.offset(begin):im.offset(begin)+len(body)]
report={'sha256':sha,'verified_body':{'begin':hex(begin),'end':hex(end),'bytes':len(body)},'instructions':rows,
 'alias':{'owner_offset':'0x30d8','subobject_offset':'0x2d0','subobject_relative_flag':'0x2e08'},
 'setter_policy':'10026b70: when flag is zero, equal cached value at subobject+27e0+index*24 skips. Nonzero flag bypasses equality; cache is written even when equal. If subobject byte2bb9 bit0 is clear, enqueue callback1001f8a0 through10026890 then OR owner byte2e89 with1.',
 'second_read':'10026a0e: nonzero flag branches to10026a57, bypassing the following cached-state comparisons. Subsequent suppression and submission paths require separate recovery.',
 'conclusion':'Observed reads force cache/publication work during restoration. They do not establish a rendering-readiness or allocation-failure gate.',
 'limits':'Full82-byte setter and displayed other-read prefix verified statically. Indexed field meaning, callback body, runtime side effects and all computed aliases remain unresolved.'}
Path('outputs/sfm64-restore-force-flag.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'verified_setter_bytes={len(body)} restore_flag_alias_verified=1 readiness_gate=not_proved')
