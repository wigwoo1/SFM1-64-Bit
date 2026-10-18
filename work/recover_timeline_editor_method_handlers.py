import re,json,struct
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');inventory=json.loads(Path('outputs/editor-qt-method-inventory.json').read_text());methods=next(o['methods'] for o in inventory['objects'] if o['class_name']=='CQTimelineEditor');spans=[]
for line in Path('work/ifm-disassembly.txt').open():
 m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
 if not m:continue
 va=int(m[1],16)
 if va>=0x1007b8c6:break
 if not(0x1007b780<=va<0x1007b834 or 0x1007b880<=va<0x1007b8c6):continue
 raw=bytes.fromhex(m[2]);at=im.offset(va);assert im.b[at:at+len(raw)]==raw
 spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
for method in methods:
 stub=im.word(0x1007b834+method['local_id']*4);method['stub']=hex(stub)
 if method['local_id']==1:method.update(dispatch='virtual',vtable_offset='0xcc');continue
 candidates=[s for s in spans if stub<=int(s['va'],16)<stub+20 and s['bytes'][:2] in ['e8','e9']]
 assert candidates,method
 s=candidates[0];va=int(s['va'],16);method.update(dispatch='direct',handler=hex(va+5+struct.unpack('<i',bytes.fromhex(s['bytes'])[1:5])[0]))
r=dict(class_name='CQTimelineEditor',static_dispatch='0x1007b780',metacall='0x1007b880',methods=methods,verified_spans=spans,limitations='Signature to direct/virtual dispatch mapping only; action bodies and enable predicates pending. Tool menu intentionally left virtual until actual class vtable identified.')
Path('outputs/timeline-editor-method-handler-recovery.json').write_text(json.dumps(r,indent=2));print(json.dumps([(m['signature'],m.get('handler',m.get('vtable_offset'))) for m in methods],indent=2))
