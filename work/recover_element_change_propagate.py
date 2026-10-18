import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
spans=[];active=None
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:
        c=re.fullmatch(r'\s{12}((?:[0-9A-F]{2} ?)+)\s*',line.rstrip())
        if c and active is not None:active['bytes']+=bytes.fromhex(c[1]).hex()
        continue
    va=int(m[1],16)
    if va>=0x1035e52f:break
    if va<0x1035e400:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x1035e400
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x1035e52f
Path('outputs/element-change-propagate-recovery.json').write_text(json.dumps(dict(complete_bytes=303,instructions=spans,findings=[
 '1035e400 service slot119 nonzero suppresses all work. Marked element early exits if active low2 OR full supplied mask equals active.',
 'Unmarked queues handle via1038f580. Filtered list44 nonempty: queue1038f5e0 when supplied mask includes required bits2..3 and existing active/state do not already satisfy required.',
 'Accumulate supplied low2 into element40, set state3e bit10. Propagated mask strips bit1 (mask AND fffffffd). Previously marked gate uses newly accumulated active low2.',
 'Incoming references service slot115/116, attributes slot89; skip unresolved, flags1000, null owner8. Recurse into owner with stripped mask; reentry state/active gate limits low2 cycles.',
 'Masks above low2 retain full-width participation in gates and propagation; termination for arbitrary unsupported high bits is not established.'
],limitations='Complete installed bytes verified; service global binding/ABI, original execution parity, unsupported mask domain and native editor integration pending.'),indent=2)+'\n')
print('Verified element change propagation bytes:',303)
