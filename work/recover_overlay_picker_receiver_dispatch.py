import re,json,struct,hashlib
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');meta=0x10e7da4c;strings=im.word(meta+4);data=im.word(meta+8);count=im.word(data+16);table=im.word(data+20);assert count==4;methods=[]
for i in range(count):
 signature=im.string(strings+im.word(data+(table+i*5)*4));stub=im.word(0x1007d970+i*4);at=im.offset(stub+4);assert im.b[at]==0xe9
 target=stub+9+struct.unpack_from('<i',im.b,at+1)[0];methods.append(dict(local_id=i,signature=signature,stub=hex(stub),handler=hex(target)))
spans=[]
for line in Path('work/ifm-disassembly.txt').open():
 m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
 if not m:continue
 va=int(m[1],16)
 if va>=0x10127780:break
 if not(0x1007d930<=va<0x1007d96e or 0x1007d9e0<=va<0x1007da26 or 0x10127750<=va<0x10127780):continue
 raw=bytes.fromhex(m[2]);at=im.offset(va);assert im.b[at:at+len(raw)]==raw
 spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
r=dict(binary=str(Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')),binary_sha256=hashlib.sha256(im.b).hexdigest(),class_name=im.string(strings),methods=methods,verified_spans=spans,verified_instruction_count=len(spans),verified_byte_count=sum(len(s['bytes'])//2 for s in spans),limitations='Listed disassembly instruction bytes are directly compared with the installed DLL. CQVMTPickerDialogHelper routes AddOverlayToTrackWithDialogAccepted to 0x101281E0 and cancellation to 0x10127750. The acceptance handler creates overlay elements; transaction-slot semantics and runtime UI behavior remain unresolved.')
Path('outputs/overlay-picker-receiver-dispatch-recovery.json').write_text(json.dumps(r,indent=2));print(json.dumps(methods,indent=2))
