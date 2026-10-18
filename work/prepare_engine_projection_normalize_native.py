from pathlib import Path
import sys,json,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll');assert hashlib.sha256(im.b).hexdigest()=='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
start=0x10244060;end=0x102440cd;code=im.b[im.offset(start):im.offset(start)+end-start];offsets=[]
for row in json.loads(Path('outputs/sfm64-engine-shadow-manager-binding.json').read_text())['instructions']:
 va=int(row['address'],16)
 if start<=va<end and 'ds:[' in row['instruction']:
  b=bytes.fromhex(row['bytes']);target=int(row['instruction'].split('ds:[')[1].split('h')[0],16);assert target==[0x1031bdec,0x10322ae8][len(offsets)];assert int.from_bytes(b[-4:],'little')==target;offsets.append(va-start+len(b)-4)
assert len(offsets)==2
Path('work/engine_projection_normalize_native_bytes.h').write_text('#pragma once\ninline constexpr unsigned char normalizeBytes[]={'+','.join(hex(b) for b in code)+'};\ninline constexpr unsigned constantOffsets[]={'+','.join(map(str,offsets))+'};\n')
p=Path('work/verify_engine_projection_storage_native.py');s=p.read_text().replace('prepare_engine_projection_storage_native.py','prepare_engine_projection_normalize_native.py').replace('projection_storage','projection_normalize').replace('projection-storage','projection-normalize').replace('65536*416','65536*28').replace('Isolated copied original storage and cached-mask routines only, with eight constant and two call relocations; full plane generation, floating-point environment variations and live rendering remain unverified.','Isolated copied original normalization routine with two constant relocations; finite-vector cases and explicit tiny/zero patterns only. NaN payloads, environment variations and full plane generation/live rendering remain unverified.')
Path('work/verify_engine_projection_normalize_native.py').write_text(s)
