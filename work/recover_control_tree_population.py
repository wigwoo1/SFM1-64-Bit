import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions=[(0x100907d0,0x10090af4),(0x105cbcf0,0x105cbd4e)]
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
assert im.string(0x10a8a7d4)=='children'
settings_at=im.offset(0x10676770)
assert im.b[settings_at:settings_at+4]==bytes.fromhex('8b4108c3')
receipt=dict(verified_prefixes=spans,findings=[
 '100907d0 resolves group with1008d2d0 and skips missing input/group.',
 'Controls storage78/count84 traversed last to first; unresolved handles and failed type10e9a684 are skipped. Shared records appended to model vector; leaf nodes prepended.',
 'Children storage5c/count68 traversed last to first. Requires resolved CDmeControlGroup and105cbcf0 visibility or10676770 result byte89 override. Override helper identity pending.',
 'Empty child groups (both count84 and count68 zero) skipped before node allocation1008d100 and recursive100907d0.',
 'Node aggregate8 promoted from zero to1 only if model record count48 increased during population.',
 '105cbcf0 recursively evaluates incoming children owner from1008cf90(owner, interned children symbol,1,0). A false ancestor short circuits; otherwise returns own visible bytea4. Parent filter now separately recovered in control-group-incoming-parent-recovery.json.',
 '10676770 complete four bytes8b4108c3 verified: returns receiver member8. Population reads returned pointer byte89 as hidden-group override. Settings object/attribute identity still pending.'
 ],limitations='Population/visibility first-line instruction prefixes only; continuation bytes not independently decoded. IsA inheritance and settings object identity require further recovery. Original runtime parity unverified.')
Path('outputs/control-tree-population-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified population/visibility prefixes:',len(spans))
