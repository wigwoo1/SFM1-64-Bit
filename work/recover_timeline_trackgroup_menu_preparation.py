import re,json
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');spans=[]
for line in Path('work/ifm-disassembly.txt').open():
 m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
 if not m:continue
 va=int(m[1],16)
 if va>=0x1016e9ef:break
 if va<0x1016e950:continue
 raw=bytes.fromhex(m[2]);at=im.offset(va);assert im.b[at:at+len(raw)]==raw
 spans.append(dict(va=hex(va),verified_byte_prefix=raw.hex(),instruction=m[3].strip()))
# Independently verify full bytes of the dynamic build scratch initialization.
at=im.offset(0x1016e9cc);assert im.b[at:at+7]==bytes.fromhex('c745f800000000')
r=dict(handler='0x1016e950',context='global10f32568->member344',guards=['context pointer nonnull','current-element resolver1027cb80 returns nonnull'],menu_order=['action memberf8','separator','action member12c','build action member128 via1016cd40 with callback101278e0','action member128'],imports=json.loads(Path('outputs/timeline-trackgroup-menu-imports.json').read_text()),verified_spans=spans,limitations='Current element resolver identity, action labels, dynamic callback, enabling rules and menu runtime behavior pending.')
Path('outputs/timeline-trackgroup-menu-preparation-recovery.json').write_text(json.dumps(r,indent=2));print('Verified menu preparation instruction prefixes:',len(spans))
Path('work/timeline_trackgroup_menu_partial_decompiled.cpp').write_text('// Offline IFM 0x1016e950 menu preparation; unknown types/labels retained.\n// context = global_context->field_344;\n// if (!context || !function_1027cb80(context)) return;\n// menu->addAction(self->action_f8);\n// menu->addSeparator();\n// menu->addAction(self->action_12c);\n// function_1016cd40(..., callback_101278e0, ...);\n// menu->addAction(self->action_128);\n')
