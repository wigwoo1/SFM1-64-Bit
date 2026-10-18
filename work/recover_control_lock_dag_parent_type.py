import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x101e3170,0x101e31f5),(0x104c5aa0,0x104c5aaf)]
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

vtable=0x10b8e4a4
assert im.word(vtable+0x14)==0x104c5aa0
locator=im.word(vtable-4);td=im.word(locator+12);name=im.string(td+8)
assert name=='.?AV?$CDmElementFactory@VCDmeDag@@@@'
receipt=dict(verified_prefixes=spans,type_global='0x10eb131c',factory_rtti=name,vtable=hex(vtable),locator=hex(locator),type_descriptor=hex(td),findings=[
'Factory RTTI independently identifies typeglobal10eb131c as CDmeDag. Incoming children-reference parent finder accepts CDmeDag-compatible owners by virtual type test.',
'Override resolver101e3170 looks up named attr; missing attr supplies invalid handleffffffff. Present attr invokes10002780 getter, reads firstwordhandle, resolvesvia data-model virtual48, rejects null or failedCDmeDag type test. Therefore overrideParent is a typed DAG reference, not arbitrary element.'
],limitations='First-line prefixes, setter-slot and RTTI association checked offline. Actual runtime type ID, attribute getter full behavior and original runtime parity pending. No claim that all DAG subclasses/relations are implemented natively.')
Path('outputs/control-lock-dag-parent-type-recovery.json').write_text(json.dumps(receipt,indent=2))
print(name,'verified prefixes:',len(spans))
