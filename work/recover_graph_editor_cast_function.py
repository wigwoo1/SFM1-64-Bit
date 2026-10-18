from pathlib import Path
import json,re
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
spans=[]
for line in Path('work/ifm-disassembly.txt').open():
 m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
 if not m:continue
 va=int(m[1],16)
 if va<0x100742d0:continue
 if va>=0x100744f7:break
 raw=bytes.fromhex(m[2]);at=im.offset(va);assert im.b[at:at+len(raw)]==raw
 spans.append(dict(address=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
classes={hex(va):im.string(va) for va in [0x10a9c8d0,0x10a9a848,0x10a9a410]}
assert im.b[im.offset(0x100742d0):im.offset(0x100742d0)+3]==bytes.fromhex('558bec')
r=dict(function_entry='0x100742d0',role='class-name cast dispatch; Qt-style metacast inferred from pattern',verified_spans=spans,classes=classes,proven_branches=[dict(input='null class-name pointer',result='null'),dict(input=classes['0x10a9c8d0'],result='this pointer'),dict(input=classes['0x10a9a848'],result='this+0x14 if this nonnull')],limitations='Partial function; remaining names and fallback tail not recovered. No editor action behavior proven.')
Path('outputs/graph-editor-cast-function-recovery.json').write_text(json.dumps(r,indent=2))
Path('work/graph_editor_cast_partial_decompiled.cpp').write_text('// Offline partial decompilation of installed IFM 0x100742d0.\n// Structural evidence only; not a complete replacement function.\n// Unknown cases intentionally excluded from this partial listing.\n// if (!name) return nullptr;\n// if (strcmp(name, "'+classes['0x10a9c8d0']+'") == 0) return self;\n// if (strcmp(name, "'+classes['0x10a9a848']+'") == 0) return self ? self + 0x14 : nullptr;\n')
print(json.dumps(dict(entry=r['function_entry'],classes=classes,verified_instruction_spans=len(spans)),indent=2))
