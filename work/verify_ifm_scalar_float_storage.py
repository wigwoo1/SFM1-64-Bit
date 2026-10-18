from pathlib import Path
from pe_image import Image
import hashlib,json
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
assert image.word(0x10362768)==0x10362736 # type3: jump table index3-2
expected={0x10362736:'8b45088b5104d900d91a5dc20400',0x1036D08D:'e81e53ffff',0x1036D0E2:'e8f955ffff',0x1036D123:'e8c81effff'}
for address,data in expected.items():
 offset=image.offset(address);assert image.b[offset:offset+len(bytes.fromhex(data))]==bytes.fromhex(data)
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),setter='0x1036d040',storage_dispatch='0x103626e0',type3_branch='0x10362736',checked_bytes={hex(k):v for k,v in expected.items()},behavior='Type3 dispatch stores incoming float through x87 fld/fstp. Setter calls mutation gate, storage dispatcher, and owner dirty helper in that order.',limits='Static installed bytes; no live native setter execution. Owned candidate excludes undo recording and derived-class notification callbacks.')
Path('outputs/sfm64-ifm-scalar-float-storage.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified native type3 storage branch and setter call targets')
