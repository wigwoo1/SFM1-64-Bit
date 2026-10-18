from pathlib import Path
from pe_image import Image
import json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
assert [image.word(0x10B387C0+4*i) for i in range(2)]==[0x10377E40,0x10377E90]
expected={0x10367B97:'e834fbcbffd9008b4d0cd95e1cd9018bc6d95e20',0x10377E71:'8a4810c0e905f6c101750b83c61c568bc8e8b951ffff',0x10377EC1:'8a4810c0e905f6c101750b83c620568bc8e86951ffff'}
for address,data in expected.items():
 offset=image.offset(address);assert image.b[offset:offset+len(bytes.fromhex(data))]==bytes.fromhex(data)
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),constructor='0x10367b20',undo='0x10377e40',redo='0x10377e90',fields={'name':'+0x14','owner_handle':'+0x18','before':'+0x1c','after':'+0x20'},behavior='Capture before/after with x87 fld/fstp; replay resolves owner and attribute name, checks flag0x20, then calls scalar float setter.',checked_bytes={hex(k):v for k,v in expected.items()},limits='Selected static spans; native undo recording gate, journal lifetime and live replay remain unverified. Owned integration pending.')
Path('outputs/sfm64-ifm-scalar-float-undo.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified scalar float undo payload capture and replay setter calls')
