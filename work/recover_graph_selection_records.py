from pathlib import Path
import json,re
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1035df40,0x1035dfcd),(0x1035ef90,0x1035efec)]
groups=[[] for _ in ranges];active=None
for line in Path('work/ifm-disassembly.txt').open():
 m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
 if not m:
  c=re.fullmatch(r'\s{12}((?:[0-9A-F]{2} ?)+)\s*',line.rstrip())
  if c and active:active['bytes']+=bytes.fromhex(c[1]).hex()
  continue
 va=int(m[1],16);active=None
 for i,(lo,hi) in enumerate(ranges):
  if lo<=va<hi:active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());groups[i].append(active);break
 if va>=ranges[-1][1]:break
for (lo,hi),spans in zip(ranges,groups):
 cur=lo
 for x in spans:
  va=int(x['va'],16);raw=bytes.fromhex(x['bytes']);assert va==cur and im.b[im.offset(va):im.offset(va)+len(raw)]==raw;cur+=len(raw)
 assert cur==hi
out=dict(verified_bytes=sum(hi-lo for lo,hi in ranges),routines=groups,findings=['1035df40 allocates a 12-byte node, stores two 32-bit arguments, links it at object+44, and adjusts packed selection bits in object+40. An optional callback is gated by byte3e bit10 and a transition condition.', '1035ef90 searches the linked nodes by the first 32-bit key, unlinks/frees a match, marks packed selection bits, and recalculates state from remaining nodes.', 'The context handler calls add with mode2 on its selected-item path and removes using the clicked element handle on the target-change path.'],limitations='Semantic names for node keys, selection bits, callback, and menu actions have not been established; report as opaque selection bookkeeping pending more callers/metadata.')
Path('outputs/graph-editor-selection-records-recovery.json').write_text(json.dumps(out,indent=2)+'\n')
print('Verified selection-record routines:',out['verified_bytes'],'bytes')

