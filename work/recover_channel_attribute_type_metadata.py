import json, re
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x10363530, 0x1036355c), (0x10363560, 0x1036359d), (0x104cfd00, 0x104cfd0f)]
spans = []
for line in Path('work/ifm-disassembly.txt').open():
    m = re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)', line)
    if not m: continue
    va = int(m[1], 16)
    if va >= regions[-1][1]: break
    if not any(a <= va < b for a,b in regions): continue
    raw = bytes.fromhex(m[2]); at = im.offset(va)
    assert im.b[at:at+len(raw)] == raw
    spans.append(dict(va=hex(va), bytes=raw.hex(), instruction=m[3].strip()))
vt = 0x10b8eab4
assert im.word(vt+0x14) == 0x104cfd00
col = im.word(vt-4); td = im.word(col+12)
name = im.string(td+8)
assert name == '.?AV?$CDmElementFactory@VCDmeChannel@@@@'
receipt = dict(verified_prefixes=spans,
    factory=dict(vtable=hex(vt), locator=hex(col), descriptor=hex(td), rtti=name, setter='0x104cfd00', type_global='0x10eb17c4'),
    findings=[
        'Channel binding routine104cbd50 passes global10eb17c4 to10363530 for both channel attributes; factory proves CDmeChannel identity.',
        '10363530 masks attribute word10 with1f; type1 stores argument into storagePointer4 plus4; type15 stores into storagePointer4 plus14; other types no-op.',
        '10363560 reads matching metadata slot for types1/15; other types write ffffffff to output.',
        'These helper bodies do not resolve handles, test object RTTI, invoke notifications or reject assigned objects. Validation at other layers remains unresolved.'
    ], limitations='Instruction first-line prefixes and factory RTTI verified against installed PE. Full continuation-byte checking, runtime type identifiers and assignment enforcement remain unverified.')
Path('outputs/channel-attribute-type-metadata-recovery.json').write_text(json.dumps(receipt, indent=2))
print('Verified prefixes:', len(spans)); print(name)
