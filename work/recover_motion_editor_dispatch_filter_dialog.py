import json,re,struct
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');inventory=json.loads(Path('outputs/editor-qt-method-inventory.json').read_text());methods=next(o['methods'] for o in inventory['objects'] if o['class_name']=='CQMotionEditorController')
handlers=[None,0x10118f40,0x10118fc0,0x10119ba0,0x10119bf0]
for i,m in enumerate(methods):m.update(dispatch_stub=hex(im.word(0x10075804+i*4)),handler=hex(handlers[i]) if handlers[i] else None,role='signal activation' if i==0 else 'slot handler')
ranges=[(0x100757b0,0x10075804),(0x10075830,0x10075875),(0x10119ba0,0x10119be8),(0x10119bf0,0x10119c38)];spans=[]
for line in Path('work/ifm-disassembly.txt').open():
 m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
 if not m:continue
 va=int(m[1],16)
 if va>=0x10119c38:break
 if not any(lo<=va<hi for lo,hi in ranges):continue
 raw=bytes.fromhex(m[2]);at=im.offset(va);assert im.b[at:at+len(raw)]==raw
 spans.append(dict(va=hex(va),verified_byte_prefix=raw.hex(),instruction=m[3].strip()))
for va in [0x10119bc8,0x10119c18]:assert im.b[im.offset(va):im.offset(va)+10]==bytes.fromhex('c7868800000000000000')
r=dict(class_name='CQMotionEditorController',methods=methods,verified_spans=spans,accept_reject=dict(virtual_completion_slot='0x4c',accept_argument=1,reject_argument=0,dialog_member='0x88',cleanup_imports=['0x10a7d230','0x10a7b30c'],dialog_pointer_cleared=True,optional_global_followup='global10f32568->member344 then102770a0'),limitations='Slot mapping and control flow recovered; virtual target, cleanup import identities and final followup semantics pending. No native dialog parity proven.')
Path('outputs/motion-editor-dispatch-filter-dialog-recovery.json').write_text(json.dumps(r,indent=2));print(json.dumps(methods,indent=2))
