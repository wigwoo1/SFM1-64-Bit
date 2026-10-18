import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
targets=[im.word(0x1008c448+i*4) for i in range(4)]
assert targets==[0x1008c403,0x1008c3f6,0x1008c3e9,0x1008c3d1]
spans=[]
for line in Path('work/ifm-disassembly.txt').open():
 m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
 if not m:continue
 va=int(m[1],16)
 if va>=0x1008c446:break
 if va<0x1008c3b0:continue
 raw=bytes.fromhex(m[2]);at=im.offset(va)
 assert im.b[at:at+len(raw)]==raw
 spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
receipt=dict(verified_prefixes=spans,jump_targets=[hex(v) for v in targets],findings=[
 'Manager164 resolves attribute for incoming iterator token; missing attribute rejected.',
 'Third stack argument is restriction selector:0 no flags gate,1 reject1000,2 require800 then reject1000,3 or unsigned>3 reject.',
 'Owner attribute8 must exist and satisfy CDmeControlGroup IsA. Requested symbol(first stack argument)ffffffff wildcard else match attribute14.',
 'Second argument low byte nonzero requires owner38 equal iterator context4 (source owner identity).',
 'Visibility caller105cbcf0 supplies children symbol, same-owner1, restriction0. It therefore ignores attribute800/1000 in this filter, but requires same owner identity.'
 ],limitations='First-line instruction prefixes plus exact jump-table words verified. Manager attribute resolution, original IsA hierarchy and original runtime parity pending.')
Path('outputs/control-group-incoming-parent-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified incoming parent prefixes:',len(spans),'jump table:',[hex(v) for v in targets])
