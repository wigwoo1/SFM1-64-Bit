import re,json
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
meta=0x10e7c99c;strings=im.word(meta+4);data=im.word(meta+8)
assert strings==0x10a9c8d0 and data==0x10a9c818
header=[im.word(data+i*4) for i in range(14)];assert header[4]==6 and header[5]==14
methods=[]
for i in range(header[4]):
 row=[im.word(data+(header[5]+i*5+k)*4) for k in range(5)]
 methods.append(dict(local_id=i,signature=im.string(strings+row[0]),parameters=im.string(strings+row[1]),return_type=im.string(strings+row[2]),tag=im.string(strings+row[3]),flags=row[4],raw=row))
spans=[]
for line in Path('work/ifm-disassembly.txt').open():
 m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
 if not m:continue
 va=int(m[1],16)
 if not (0x10074250<=va<0x100742c0 or 0x10074510<=va<0x10074556 or 0x10074560<=va<0x10074570):continue
 raw=bytes.fromhex(m[2]);at=im.offset(va);assert im.b[at:at+len(raw)]==raw
 spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
r=dict(class_name=im.string(strings),metaobject=hex(meta),metadata_revision=header[0],methods=methods,metacall_entry='0x10074510',static_dispatch='0x10074250',metaobject_accessor='0x10074560',verified_spans=spans,dispatch_behavior='Calls base dispatch first; negative IDs return. Call type zero dispatches local IDs below6 then subtracts6; other call types preserve base result.',limitations='Method signatures and wrapper recovered; slot bodies, context-menu contents and action behavior still unverified.')
Path('outputs/graph-editor-method-dispatch-recovery.json').write_text(json.dumps(r,indent=2));print(json.dumps(methods,indent=2))
