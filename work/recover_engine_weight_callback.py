from pathlib import Path
from pe_image import Image
import re,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
lines=Path('work/engine-disassembly.txt').read_text(encoding='utf-8').splitlines();instructions=[]
for line in lines:
    match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
    if match: instructions.append([int(match[1],16),bytes.fromhex(match[2]),line.strip()])
    elif instructions:
        tail=re.fullmatch(r'\s+((?:[0-9A-F]{2} ?)+)\s*',line)
        if tail: instructions[-1][1]+=bytes.fromhex(tail[1])
checked=[]
for address,data,line in instructions:
    if not 0x100DAD60<=address<0x100DAD88:continue
    offset=image.offset(address);assert image.b[offset:offset+len(data)]==data
    checked.append(dict(address=hex(address),bytes=data.hex(),instruction=line))
assert len(checked)==15
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),call='0x100dad84',object_source='[esi+8]',method_byte_offset='0x38',zero_based_slot=14,
    arguments=['[ebp-258h]: bones candidate','[esi] then +104h: weight count candidate','ebx: first allocated weight buffer','[ebp-254h]: optional second allocated weight buffer'],
    gate='Only called when ebx (first allocation) is non-null',verified_full_instructions=checked,
    public_comparison=dict(url='https://raw.githubusercontent.com/ValveSoftware/source-sdk-2013/master/src/public/iclientrenderable.h',method='SetupWeights',public_slot=17,signature='bones, count, flex weights, delayed flex weights'),
    limitations='Matching four-argument roles support SetupWeights identification, but installed object class/table and layout difference remain unresolved. No callback execution or buffer contents inspected.')
Path('outputs/sfm64-engine-weight-callback.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print('Verified',len(checked),'complete instructions; weight callback slot 14')
