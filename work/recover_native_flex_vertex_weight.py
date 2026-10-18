from pathlib import Path
from pe_image import Image
import re,struct,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll')
ranges=[(0x10009c20,0x10009d7e),(0x10012160,0x100123b0),(0x10012630,0x10012680)]
lines=[];checked=0
for line in Path('work/studiorender-disassembly.txt').read_text().splitlines():
    match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
    if not match:continue
    address=int(match[1],16)
    if not any(first<=address<last for first,last in ranges):continue
    prefix=bytes.fromhex(match[2]);offset=image.offset(address)
    assert image.b[offset:offset+len(prefix)]==prefix
    checked+=1;lines.append(line)
unit=struct.unpack_from('<f',image.b,image.offset(0x10070524))[0]
assert unit==struct.unpack('<f',struct.pack('<f',1/255))[0]
Path('outputs/sfm64-native-flex-vertex-weight-disassembly.txt').write_text('\n'.join(lines)+'\n',encoding='utf-8')
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),instruction_prefixes_verified=checked,byte_weight_scale=unit,
    evidence='10009C20 selects descriptor and pair weights from owner+A8/AC; pair zero copies primary weights; wrapper10012630 calls10012160; record+2 is speed and record+3 side;100122DB-10012314 interpolates each descriptor with speed then between descriptors with side.',
    limitations='Offline candidate; temporal identity/lifetime of owner arrays, quantized-delta conversion and native runtime oracle unverified. Prefix checks do not validate every continuation byte.')
Path('outputs/sfm64-native-flex-vertex-weight-recovery.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(f'Native vertex weight evidence: {checked} instruction prefixes checked, byte scale={unit}')
