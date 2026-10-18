import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x100d61d0,0x100d61f1),(0x100bf630,0x100bf63d)];spans=[]
for line in Path('work/ifm-disassembly.txt').open():
 m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
 if not m:continue
 va=int(m[1],16)
 if va>=0x100d61f1:break
 if not any(lo<=va<hi for lo,hi in ranges):continue
 raw=bytes.fromhex(m[2]);at=im.offset(va);assert im.b[at:at+len(raw)]==raw
 spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
construction=json.loads(Path('outputs/graph-editor-keymode-construction-recovery.json').read_text())
callback_spans=[s for s in construction['construction_window']['verified_spans'] if 0x100d8dd3<=int(s['va'],16)<=0x100d8df3 or 0x100d8e78<=int(s['va'],16)<=0x100d8e99 or 0x100d8f1b<=int(s['va'],16)<=0x100d8f3c or 0x100d8fc0<=int(s['va'],16)<=0x100d8feb]
r=dict(common_callback='0x100d61d0',callback_behavior='Pass argument to setter100bf630(this+150), then call UI refresh100d4e40(this)',setter='0x100bf630',setter_behavior='Store argument at receiver+c; window effective offset15c',verified_spans=spans,construction_callback_spans=callback_spans,limitations='Callback allocation/parameter bindings partially inspected; generic callback invocation and refresh body pending. Native action execution not yet tested.')
Path('outputs/graph-editor-keymode-selection-recovery.json').write_text(json.dumps(r,indent=2))
Path('work/graph_editor_keymode_selection_decompiled.cpp').write_text('// Verified IFM 0x100d61d0 and setter0x100bf630.\n// void SetAddKeyMode(int mode) {\n//   *(int*)(self + 0x15c) = mode;\n//   function_100d4e40(self); // UI refresh semantics pending\n// }\n')
print('Verified selection callback and mode setter:',len(spans),'instruction spans')
