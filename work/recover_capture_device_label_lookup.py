import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions=[(0x1025f610,0x1025f65c),(0x1025f760,0x1025f80e),(0x1025fa30,0x1025fa3c)]
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
 '1025fa30 forwards to1025f760 with receiver member4.',
 '1025f610 validates signed index against manager member210, reads encoded slot from204. Low15bits index; bit15 chooses pointer table1f0 else inline pointer table70. Upper16bits ignored.',
 '1025f760 rejects null sought element, scans logical slots ascending, resolves record handle0 through manager virtual48 then requires CDmeAnimationSet before pointer equality. Returns first matching logical slot or-1. Count210 reread after each unsuccessful comparison.',
 'Display label reads selected recordf4:nonnegative produces sixense(index+1); negative uses recordf8 mocap number. Dynamic/fixed record ownership and metadata initialization still pending.'
 ],limitations='First-line instruction prefixes only. Full capture record lifecycle/formatting helper semantics and runtime parity unverified.')
Path('outputs/capture-device-label-lookup-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified capture lookup prefixes:',len(spans))
