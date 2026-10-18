import re,json
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');spans=[]
for line in Path('work/ifm-disassembly.txt').open():
 m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
 if not m:continue
 va=int(m[1],16)
 if va>=0x1011903d:break
 if not(0x10118f40<=va<0x10118fbd or 0x10118fc0<=va<0x1011903d):continue
 raw=bytes.fromhex(m[2]);at=im.offset(va);assert im.b[at:at+len(raw)]==raw
 spans.append(dict(va=hex(va),verified_byte_prefix=raw.hex(),instruction=m[3].strip()))
r=dict(types={hex(va):im.string(va+8) for va in [0x10e8153c,0x10e84398,0x10e843c0]},handlers=dict(smooth='0x10118f40',jitter='0x10118fc0'),behavior='Require dialog member88 and successful operation cast of member30; read float index0 via100fe6e0 and second value index1 via100fe780; on both success store second word at operation+48(smooth)/+4c(jitter), then virtualslot58(first float).',verified_spans=spans,limitations='Input getter semantics/validation and actual filter algorithm pending; no smoothing/jitter numeric parity proven.')
Path('outputs/motion-editor-smooth-jitter-dialog-recovery.json').write_text(json.dumps(r,indent=2));print(json.dumps(dict(types=r['types'],verified_spans=len(spans)),indent=2))
