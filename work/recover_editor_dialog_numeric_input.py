import re,json
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');spans=[]
for line in Path('work/ifm-disassembly.txt').open():
 m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
 if not m:continue
 va=int(m[1],16)
 if va>=0x100fe7b5:break
 if not(0x100fe6e0<=va<0x100fe77a or 0x100fe780<=va<0x100fe7b5):continue
 raw=bytes.fromhex(m[2]);at=im.offset(va);assert im.b[at:at+len(raw)]==raw
 spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
report=dict(float_getter='0x100fe6e0',integer_getter='0x100fe780',array_layout=dict(pointer_offset='0x20',count_offset='0x2c',stride=8,type_offset=0,widget_offset=4),float_behavior='Reject signed index<0 or index>=count; type5 QLineEdit::text then QString::toFloat(&ok); failure leaves output unchanged; type6 QDoubleSpinBox::value double rounded/stored as float and true; other types false.',integer_behavior='Same index bounds; require type7 QSpinBox::value and write int32; other types false/output unchanged.',verified_spans=spans,imports=json.loads(Path('outputs/editor-ui-input-import-semantics.json').read_text()),limitations='Installed control flow/import APIs proven; Qt text conversion edge cases, widget validators/ranges and native execution parity pending.')
Path('outputs/editor-dialog-numeric-input-recovery.json').write_text(json.dumps(report,indent=2));print('Verified numeric getter instructions:',len(spans))
Path('work/editor_dialog_numeric_input_decompiled.cpp').write_text('// Recovered dialog numeric getter behavior (Qt4 API names verified).\n// float: bounds -> type5 text().toFloat(&ok) writes only if ok;\n// type6: float(QDoubleSpinBox::value()) succeeds; otherwise false.\n// int: bounds -> type7 QSpinBox::value() writes int32; otherwise false.\n// No std::stof substitution: Qt parsing/validators must be preserved.\n')
