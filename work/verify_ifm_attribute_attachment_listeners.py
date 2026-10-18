from pathlib import Path
from pe_image import Image
import hashlib,json
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
expected={0x10360811:'8b90dc010000ffd284c00f855c010000',0x10360828:'8b7b54',0x103608F3:'8b178911',0x10360904:'8b4f04894dfc85c90f8543ffffff',0x10360920:'8b0cb28b018b1053ffd2',0x103616F2:'e809f1ffff'}
expected.update({0x1035E546:'8b4d088908c74004000000008b5654895004894654',0x1035F983:'8b55088d4154',0x1035F99A:'391174058d4104ebed',0x1035F9A7:'8b510451b9409bf4108910e8d9af6100'})
for address,data in expected.items():
 offset=image.offset(address);assert image.b[offset:offset+len(bytes.fromhex(data))]==bytes.fromhex(data)
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),routine='0x10360800',binding_call='0x103616f2',suppression='datamodel virtual offset0x1dc returns nonzero -> exit',listeners='owner+0x54 list, node listener at+0, next at+4',dispatch='Snapshot listener pointers in traversal order; call each listener virtual slot0 with original attribute pointer.',checked_bytes={hex(k):v for k,v in expected.items()},limits='Selected static spans. Listener registration, suppression method name, listener lifetimes and downstream native effects remain unverified. Does not establish final attribute flags after listeners run.')
report.update(registration='0x1035e530: allocate 8-byte pool node, store listener, prepend at owner+0x54; duplicates permitted',removal='0x1035f980: pointer-to-link traversal, unlink first matching listener and free node',limits='Selected static spans. Suppression method name, listener lifetimes and downstream native effects remain unverified. Does not establish final attribute flags after listeners run.')
Path('outputs/sfm64-ifm-attribute-attachment-listeners.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified attribute attachment listener snapshot and callback sites')
