import json,re,struct
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
r=json.loads(Path('outputs/editor-qt-method-inventory.json').read_text());targets={int(o['metaobject_va'],16):o for o in r['objects']};accessors=[]
for line in Path('work/ifm-disassembly.txt').open():
 m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
 if not m:continue
 raw=bytes.fromhex(m[2]);va=int(m[1],16)
 if len(raw)!=5 or raw[0]!=0xb8:continue
 target=struct.unpack('<I',raw[1:])[0]
 if target not in targets:continue
 at=im.offset(va);assert im.b[at:at+5]==raw
 # Recognize the same complete 16-byte metaobject accessor proven for graph window.
 start=va-10;at=im.offset(start);body=im.b[at:at+16]
 if body[:10]!=bytes.fromhex('8b41048b401885c07505') or body[-1]!=0xc3:continue
 found=[];pos=0
 while True:
  pos=im.b.find(struct.pack('<I',start),pos)
  if pos<0:break
  off=pos;pos+=1
  sec=next(((rv,sz,ro) for rv,sz,ro in im.sections if ro<=off<ro+sz),None)
  if not sec:continue
  rv,sz,ro=sec;table=im.base+rv+off-ro
  if table<0x10a80000:continue
  cast=im.word(table+4);call=im.word(table+8)
  if not (im.base+0x1000<=cast<0x10a80000 and im.base+0x1000<=call<0x10a80000):continue
  found.append(dict(vtable_slot_va=hex(table),metacast=hex(cast),metacall=hex(call)))
 accessors.append(dict(class_name=targets[target]['class_name'],metaobject=hex(target),accessor=hex(start),verified_accessor_bytes=body.hex(),dispatch_table_candidates=found,method_count=len(targets[target]['methods'])))
assert any(a['class_name']=='CQGraphEditorWindow' and any(t['metacall']=='0x10074510' for t in a['dispatch_table_candidates']) for a in accessors)
report=dict(classes=accessors,limitations='Complete accessor bytes verified. Vtable locations are initial candidates; selected Graph Editor, CQSFMMainWindow Additional Editors menu, and CQSFMController editor-window handlers are independently byte-verified in focused audit artifacts.')
Path('outputs/editor-qt-dispatch-location-inventory.json').write_text(json.dumps(report,indent=2));print(json.dumps(dict(classes=len(accessors),candidates=sum(len(a['dispatch_table_candidates']) for a in accessors),sample=accessors[:4]),indent=2))
