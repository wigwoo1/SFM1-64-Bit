import json,hashlib
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
r=json.loads(Path('outputs/sfm-time-to-frame-recovery.json').read_text())
base=0x1097af20
blob=bytearray([0xcc])*(0x10a16b24-base)
for group in r['routines']:
    for s in group:
        va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
        assert im.b[im.offset(va):im.offset(va)+len(raw)]==raw
        blob[va-base:va-base+len(raw)]=raw
Path('work/time-to-frame-original-code.bin').write_bytes(blob)
print('Verified isolated routine bundle; relative helper displacements preserved',len(blob),hashlib.sha256(blob).hexdigest())
