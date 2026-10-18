from pathlib import Path
import json,re
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
r=(0x100d6cb0,0x100d6ce9);sp=[];active=None
for line in Path('work/ifm-disassembly.txt').open():
 m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
 if not m:
  c=re.fullmatch(r'\s{12}((?:[0-9A-F]{2} ?)+)\s*',line.rstrip())
  if c and active:active['bytes']+=bytes.fromhex(c[1]).hex()
  continue
 va=int(m[1],16);active=None
 if r[0]<=va<r[1]:active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());sp.append(active)
 if va>=r[1]:break
cur=r[0]
for x in sp:
 va=int(x['va'],16);raw=bytes.fromhex(x['bytes']);assert va==cur and im.b[im.offset(va):im.offset(va)+len(raw)]==raw;cur+=len(raw)
assert cur==r[1]
out=dict(verified_bytes=r[1]-r[0],routine=[sp],findings=['Walks stored 32-bit selected element handles in reverse order.', 'For each item, if model service global exists, calls element-handle cleanup1035d9d0(handle,0).', 'Always resets byte flag28 and count0c after cleanup.'],limitations='Meaning of byte flag and list role are inferred from the immediately connected context handler; full selection-state lifecycle remains under recovery.')
Path('outputs/graph-editor-selection-clear-recovery.json').write_text(json.dumps(out,indent=2)+'\n');print('verified',cur-r[0],'bytes')
