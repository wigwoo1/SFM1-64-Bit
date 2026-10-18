from pathlib import Path
from pe_image import Image
import json,re,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll');table=0x100710DC;entry=0x1007114C
assert image.word(entry)==0x10057340
locator=image.word(table-4);type_descriptor=image.word(locator+12);name=image.string(type_descriptor+8)
lines=[]
for line in Path('work/studiorender-disassembly.txt').read_text(encoding='utf-8').splitlines():
 match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
 if not match or '100710DCh' not in line:continue
 prefix=bytes.fromhex(match[2]);offset=image.offset(int(match[1],16));assert image.b[offset:offset+len(prefix)]==prefix;lines.append(line.strip())
assert lines
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),vtable=hex(table),entry_address=hex(entry),zero_based_slot=(entry-table)//4,function='0x10057340',rtti_locator=hex(locator),type_descriptor=hex(type_descriptor),decorated_type_name=name,verified_vtable_stores=lines,limitations='PE32 MSVC RTTI/table evidence; interface signature and temporal meaning not inferred solely from class or slot. No runtime dispatch oracle; wrapped continuation bytes excluded.')
Path('outputs/sfm64-flex-caller-rtti.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(name,'slot',(entry-table)//4)
