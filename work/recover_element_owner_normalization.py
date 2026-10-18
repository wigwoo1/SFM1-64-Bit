import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
spans=[]
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:continue
    va=int(m[1],16)
    if va>=0x109762d0:break
    if not (0x10353980<=va<0x10353a28 or 0x10976240<=va<0x10976262 or 0x10976270<=va<0x109762d0):continue
    raw=bytes.fromhex(m[2]);at=im.offset(va)
    assert im.b[at:at+len(raw)]==raw
    spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
absolute=bytes.fromhex('558bec8b4d088a0184c074068079013a740c3c2f74083c5c740432c05dc3b0015dc3')
assert im.b[im.offset(0x10976240):im.offset(0x10976262)]==absolute
report=dict(verified_prefixes=spans,complete_absolute_predicate_bytes=absolute.hex(),findings=[
 '10353980 first normalizes source via10976270 into104-byte local, then tests10976240.',
 '10976240 accepts first byte slash/backslash or nonempty string with second byte colon. Does not require drive letter or a slash after colon.',
 'Relative branch calls service10f4c76c virtual98 with second104-byte local, then10976910(output,capacity,normalized,service buffer). Absolute branch copies via109756f0.',
 '10976270 calls10a16550 copy then forces capacity-1 terminator when signed capacity>0, converts slash/backslash to backslash, calls10975f80(buffer,backslash),10975d20(buffer),10a64a90(buffer).',
 'Whole semantics depend on service buffer and three postprocessors; std::filesystem normalization not equivalent evidence.'
],limitations='Instruction prefixes and full34-byte absolute predicate verified. Copy/cleanup/case or locale postprocessors and relative resolver/service interface semantics unverified; full normalization parity not claimed.')
Path('outputs/element-owner-normalization-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified owner normalization prefixes:',len(spans),'complete absolute predicate bytes:',len(absolute))
