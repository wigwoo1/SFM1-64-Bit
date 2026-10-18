from pathlib import Path
from pe_image import Image
import json,hashlib,struct
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf\bin\client.dll')
source=json.loads(Path('outputs/sfm64-client-weight-candidate.json').read_text(encoding='utf-8'))
assert source['binary_sha256']==hashlib.sha256(image.b).hexdigest()
instructions=[]
for item in source['verified_complete_instructions']:
    address=int(item['address'],16)
    if not 0x10048C8D<=address<=0x10048E57:continue
    data=bytes.fromhex(item['bytes']);offset=image.offset(address)
    assert image.b[offset:offset+len(data)]==data
    instructions.append(item)
constants=[]
for address,kind in [(0x104BA584,'f'),(0x104B9EE0,'f'),(0x104C8D80,'d'),(0x104C8D78,'f')]:
    offset=image.offset(address);size=struct.calcsize(kind);data=image.b[offset:offset+size]
    constants.append(dict(address=hex(address),bytes=data.hex(),value=struct.unpack('<'+kind,data)[0]))
report=dict(binary_sha256=source['binary_sha256'],constants=constants,verified_instructions=instructions,
    blend='history[i] = float32(float32(current[i] * float32(1 - coefficient)) + float32(history[i] * coefficient))',
    history_pointer_field='renderable_this+0xd58',optional_output_argument='ebp+0x14',
    coefficient_limitations='Reads a global object field +10h, with calls to 1045D660 and 1045D509. Helper identities and frame-time field semantics remain unresolved.',
    limitations='Arithmetic/store evidence only. Copy-helper identity, initialization, repeated-time behavior, seeking, derived overrides and SFM runtime dispatch not yet established.')
Path('outputs/sfm64-client-delayed-blend-recovery.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps(constants));print('Verified delayed blend window:',len(instructions),'instructions')
