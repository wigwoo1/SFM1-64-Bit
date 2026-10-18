from pathlib import Path
import re,json,hashlib
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\materialsystem.dll')
assert im.word(0x1009c9dc+18*4)==0x10037d40
regions=[(0x10037d40,0x10037f35)]
lines=Path('work/materialsystem-disassembly.txt').read_text().splitlines()
results=[]
for start,end in regions:
 covered={};address=None;instructions=0
 for line in lines:
  m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)',line)
  if m:
   address=int(m[1],16);raw=bytes.fromhex(m[2])
   if start<=address<end:instructions+=1
  elif address is not None and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):raw=bytes.fromhex(line)
  else:continue
  for b in raw:
   if start<=address<end:covered[address]=b
   address+=1
 assert len(covered)==end-start
 raw=bytes(covered[a] for a in range(start,end))
 assert raw==im.b[im.offset(start):im.offset(start)+len(raw)]
 results.append(dict(start=hex(start),end=hex(end),bytes=len(raw),instructions=instructions))
report=dict(sha256=hashlib.sha256(im.b).hexdigest(),regions=results,vtable='0x1009c9dc',slot=18,draw='0x10037d40',flow=[
'ShaderAPI global1011b098 slot202 called before shader slot7(parameters,ShaderAPI); resulting index OR alpha argument.',
'Snapshot array at shaderState+24, entry stride44. Full32bit index used for initial word0 availability gate; zero exits immediately.',
'Parameter0 slot26 returns flags; debug globals100fc800/801 select helper10037890 unless flags bit8. Globals100fc807 or100fc805==2 select helper100378e0 if parameter2 flags bit8/bit16.',
'Lowbyte mode argument plus parameter2 flags bit23 controls warning and fallback100376f0(global101059ec,compression). Debug/fallback paths converge at reset call.',
'Normal path ShaderAPI slot6; flags bit1000 invokes slot100(0). Calls three imports10095148/14c/150, stores shaderState+84/current snapshot and truncates index into systembyte90; clears byte91.',
'Normal path uses truncated8bit index for subsequent snapshot accesses; snapshot word+4 truncated16 passed to ShaderAPI slot93.',
'Context pointer at snapshot+12; nonnull context whose word+4 differs from decoded argument gets dirty byte+8=1 then word+4 updated. Identical argument preserves dirty byte.',
'Shader slot6 receives(parameters,full32bit snapshot index,0,ShaderAPI,compression,addressOfContextPointer,snapshot+28).',
'Import10095144(1), helper10076bf0(system+28), clear system+84, then global100fc7b0 slot135(0). Early missing-snapshot return skips this reset.'
],limitations='Exact static bytes and RTTI table checked. Shader public ABI, debug helper behavior, import identities, snapshot ownership, shader callback binding and GPU execution remain unverified.')
Path('outputs/sfm64-shader-system-draw-recovery.json').write_text(json.dumps(report,indent=2))
print('checked_shader_system_draw_bytes='+str(sum(r['bytes'] for r in results)))
