import json,re,struct
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
r=json.loads(Path('outputs/graph-editor-method-dispatch-recovery.json').read_text())
for m in r['methods']:
 stub=im.word(0x100742b0+m['local_id']*4);at=im.offset(stub);raw=im.b[at:at+28]
 # The verified stubs have one direct relative call or tail jump to the handler.
 source=[s for s in r['verified_spans'] if stub<=int(s['va'],16)<stub+20 and s['bytes'][:2] in ['e8','e9']]
 assert source
 s=source[0];va=int(s['va'],16);target=va+5+struct.unpack('<i',bytes.fromhex(s['bytes'])[1:5])[0]
 m.update(dispatch_stub=hex(stub),handler=hex(target))
imports={};pe=im.u32(60);opt=pe+24;desc=im.base+im.u32(opt+104)
while im.word(desc):
 original=im.word(desc);library=im.string(im.base+im.word(desc+12));iat=im.word(desc+16);n=0
 while im.word(im.base+original+n*4):
  value=im.word(im.base+original+n*4)
  if not value&0x80000000:imports[im.base+iat+n*4]=dict(library=library,name=im.string(im.base+value+2))
  n+=1
 desc+=20
spans=[]
for line in Path('work/ifm-disassembly.txt').open():
 m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
 if not m:continue
 va=int(m[1],16)
 if va>=0x100d4f43:break
 if va<0x100d4ee0:continue
 raw=bytes.fromhex(m[2]);at=im.offset(va);assert im.b[at:at+len(raw)]==raw
 spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
assert imports[0x10a7cac0]['name'].find('setChecked')>=0
report=dict(methods=r['methods'],keymode_preparation=dict(entry='0x100d4ee0',mode_getter='0x100a2b20',mode_receiver='this+0x150',actions=[dict(object_offset=hex(0xcc+i*4),checked_when_mode_equals=i) for i in range(4)],import_call=imports[0x10a7cac0],verified_spans=spans),limitations='Menu checked-state preparation recovered; mode labels, getter semantics, enabled-state predicates and selection handlers pending.')
Path('outputs/graph-editor-keymode-handler-recovery.json').write_text(json.dumps(report,indent=2))
Path('work/graph_editor_keymode_decompiled.cpp').write_text('// Byte-verified offline decompilation of IFM 0x100d4ee0.\n// Types remain provisional; QObject member offsets are x86-specific.\n// int mode = function_100a2b20(self + 0x150);\n// QAction_at(self, 0xcc)->setChecked(mode == 0);\n// QAction_at(self, 0xd0)->setChecked(mode == 1);\n// QAction_at(self, 0xd4)->setChecked(mode == 2);\n// QAction_at(self, 0xd8)->setChecked(mode == 3);\n')
print(json.dumps(dict(handlers=[(m['signature'],m['handler']) for m in report['methods']],checked_setter=imports[0x10a7cac0],handler_spans=len(spans)),indent=2))
