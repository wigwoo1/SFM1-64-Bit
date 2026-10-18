from pathlib import Path
import hashlib,re,struct,json
from pe_image import Image
path=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll')
image=Image(path);lines=Path('work/studiorender-disassembly.txt').read_text().splitlines()
selected=[];checked=0
for line in lines:
    match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
    if not match:continue
    address=int(match[1],16)
    if not 0x10012e70<=address<0x10012efc:continue
    prefix=bytes.fromhex(match[2]);offset=image.offset(address)
    assert image.b[offset:offset+len(prefix)]==prefix
    selected.append(line);checked+=1
constant=struct.unpack_from('<f',image.b,image.offset(0x100701f8))[0]
assert constant==1.0
Path('outputs/sfm64-native-flex-ramp-disassembly.txt').write_text('\n'.join(selected)+'\n',encoding='utf-8')
report=dict(binary=str(path),binary_sha256=hashlib.sha256(image.b).hexdigest(),helper_va='0x10012e70',instruction_prefixes_verified=checked,plateau_constant=constant,
    candidate_semantics='For finite ordered targets: zero when value<=target0 or value>=target3; rising (value-target0)/(target1-target0) below target1; falling (target3-value)/(target3-target2) above target2; otherwise one.',
    limitations='Offline helper recovery; call-site ownership, descriptor/pair selection, side/speed blending and live native oracle remain unverified. Instruction prefixes checked; not an exhaustive control-flow validation.')
Path('outputs/sfm64-native-flex-ramp-recovery.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps(report,indent=2))
