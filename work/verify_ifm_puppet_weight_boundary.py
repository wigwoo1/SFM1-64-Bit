from pathlib import Path
from pe_image import Image
import json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
assert image.word(0x10ADB904+14*4)==0x10240630
assert image.word(0x10ADB904+39*4)==0x1097C360
checks=[(0x1097C360,'B0 01 C3'),(0x10240647,'8B 45 08'),(0x1024064A,'8B 55 14'),(0x10240655,'8B 4D 10'),(0x10240659,'8D 7E FC')]
for address,expected in checks:
 data=bytes.fromhex(expected);offset=image.offset(address);assert image.b[offset:offset+len(data)]==data
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),class_name='CDmePuppet',renderable_table='0x10adb904',weight_slot=14,weight_function='0x10240630',delayed_predicate_slot=39,delayed_predicate='0x1097c360',predicate_result=True,verified_bytes=[dict(address=hex(address),bytes=expected) for address,expected in checks],limitations='Static RTTI/slot boundary and exact selected bytes. Puppet construction, runtime engine dispatch and full weight/history algorithm remain unverified. Name-filter scan includes unrelated short tables and is not complete interface coverage.')
Path('outputs/sfm64-ifm-puppet-weight-boundary.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print('Verified CDmePuppet callback differs from client BaseFlex; slot39 returns true')
