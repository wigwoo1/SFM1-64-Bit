import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x105dd0a0, 0x105dd2ed)]
spans = []
for line in Path('work/ifm-disassembly.txt').open():
    m = re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)', line)
    if not m:
        continue
    va = int(m[1], 16)
    if va >= regions[-1][1]:
        break
    if not any(a <= va < b for a, b in regions):
        continue
    raw = bytes.fromhex(m[2])
    at = im.offset(va)
    assert im.b[at:at + len(raw)] == raw
    spans.append(dict(va=hex(va), bytes=raw.hex(), instruction=m[3].strip()))

rows=[(0x10abd754,0xa4,5),(0x10b97874,0xac,8),(0x10ace900,0xb4,2),(0x10b97870,0xbc,2),(0x10ac25fc,0xc4,2),(0x10ac25f4,0xcc,2),(0x10b97864,0xd4,4),(0x10b97858,0xdc,4),(0x10b97848,0x104,3),(0x10b97838,0x10c,3),(0x10b9782c,0xe4,2),(0x10b97820,0xec,2),(0x10b97810,0xf4,2),(0x10b97800,0xfc,2)]
attrs=[dict(name=im.string(va),value_offset=hex(offset),type_argument=kind) for va,offset,kind in rows]
receipt=dict(verified_prefixes=spans,attributes=attrs,
    explicit_defaults_by_offset={'0xac':'0xffffffff packed word','0xd4':True,'0xbc':0,'0xcc':1},
    auxiliary_strings={hex(va):im.string(va) for va in [0x10ad8bc0,0x10ad8924,0x10ad7f44,0x10ad8bd4,0x10b977f8]},
    limitations='Full construction body instruction prefixes checked, continuation bytes not comprehensively checked. Additional assignments pass pointers into existing wrappers rather than literal values; their defaults require further proof. Attribute type arguments raw. Auxiliary material construction APIs not independently identified; native/runtime parity pending.')
Path('outputs/overlay-effect-construction-recovery.json').write_text(json.dumps(receipt,indent=2))
print(json.dumps(dict(attributes=attrs,auxiliary_strings=receipt['auxiliary_strings'],verified_prefix_count=len(spans)),indent=2))
