import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x1007ef90,0x1007f37c)]
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

switch_bytes=list(im.b[im.offset(0x1007f388):im.offset(0x1007f388)+9])
targets=[im.word(0x1007f37c+4*i) for i in range(max(switch_bytes)+1)]
labels={hex(a):im.string(a) for a in [0x10aa5200,0x10aa5220,0x10aa5240,0x10aa5350,0x10aa51dc,0x10aa51e8]}
receipt=dict(verified_prefixes=spans,entry='0x1007ef90',labels=labels,column1_kind_dispatch={str(i+1):hex(targets[v]) for i,v in enumerate(switch_bytes)},findings=[
'Null node or role other than10 returnsfalse. Values converted through QVariant::toInt with null ok pointer; equality2 means true.',
'Column2 resolves1008d220(node,true), calls101d5ee0(target,bool), returns true. Kind2 fallback resolves auxiliary DAG chain; existing target yields true, newly created fallback path closes data-model scope then returns false even after a setter call.',
'Column3 resolved control group uses selectable/unselectable scope labels, calls105cb400(bool), updates ancestors1007e980 and descendants1007ea00(column3), closes scope and returnstrue. Kind1 fallback calls1008d460 recursively, emits same model updates, returnstrue.',
'Column1 uses kind-indexed two-level byte/dword switch. Active branch reads two booleans104c3e30, toggles both for kind4, first forkind5, second forkind9, invokes103fbfa0 within scope then1007ed40 and returnstrue.'
],limitations='First-line prefixes checked against actual bytes. Switch data decoded as data, excluded from instruction range. Callee object identities and setter semantics beyond direct evidence remain pending; data-model scope naming and full runtime/UI parity unverified.')
Path('outputs/control-model-edit-routine-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans));print(receipt['column1_kind_dispatch']);print(labels)
