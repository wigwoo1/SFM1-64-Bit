import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x1008d350,0x1008d3a4)]
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

receipt=dict(verified_prefixes=spans,findings=[
'1008d350 reads directnode controlrecord+10; missingrecord returnsnull without writing optional outputs. No ancestor search.',
'First optional output gets kindmask & record+4 availabilitybits, secondoptionaloutput gets kindmask & record+8 selectionbits. Both writes occur before data-model handle resolution and thus remain updated even if handle resolution returnsnull.',
'Return resolves record firstwordhandle via data-model virtual48; no typeguard in this function. Caller subsequently validates transformcontrol/DAG.'
],limitations='First-line prefixes checked. Availabilitybits meaning inferred from constructor mask use; actual UI policy and originalruntime parity unresolved. Kindmapping restricted to constructor-proven domain in nativecandidate.')
Path('outputs/control-model-control-record-lookup-recovery.json').write_text(json.dumps(receipt,indent=2));print('Verified prefixes:',len(spans))
