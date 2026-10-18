import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions=[(0x10188650,0x101886c4),(0x105cc380,0x105cc4e3)]
spans=[]
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:continue
    va=int(m[1],16)
    if va>=regions[-1][1]:break
    if not any(a<=va<b for a,b in regions):continue
    raw=bytes.fromhex(m[2]);at=im.offset(va)
    assert im.b[at:at+len(raw)]==raw
    spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
receipt=dict(verified_prefixes=spans,findings=[
    '105cc380 gathers handles from group children array5c/count68 in order, resolves manager virtual48 and retains only CDmeControlGroup objects confirmed virtual10.',
    'Boolean argument false chooses105cb3a0 name comparator; true chooses103c1480 whose meaning remains pending. Sort wrapper10188650 receives temporary pointer vector.',
    '10188650 count<=1 no-op; nonnull contiguous storage calls10a12730(data,count,4,comparator). Fallback branch uses adjacent comparisons/swaps when result negative; ordinary sort routine identity/tie policy pending.',
    'After sort,105cc380 calls103779a0 on children wrapper, then for each nonnull sorted object obtains handle10335830 and appends via10377870. Invalid/missing original entries disappear from resulting array.',
    'Temporary buffer freed afterward when owned. Array mutation helper callbacks and exact ordering for equal names unresolved.'
],limitations='First-line prefixes verified against installed PE. Alternate comparator, sorting algorithm/ties, array mutation notifications and complete runtime/native UI parity unresolved.')
Path('outputs/control-group-child-sort-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
