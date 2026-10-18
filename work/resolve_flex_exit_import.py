from pathlib import Path
from pe_image import Image
import struct,json,hashlib,re
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll')
pe=image.u32(60);optional=pe+24
directory=image.u32(optional+104);size=image.u32(optional+108)
assert directory and size>=20
imports=[]
for n in range(size//20):
 descriptor=image.offset(image.base+directory+n*20)
 original,timestamp,forwarder,name,iat=struct.unpack_from('<5I',image.b,descriptor)
 if not any((original,timestamp,forwarder,name,iat)):break
 dll=image.string(image.base+name);source=original or iat
 for index in range(65536):
  value=image.word(image.base+source+index*4)
  if not value:break
  symbol=f'ordinal:{value&0xffff}' if value&0x80000000 else image.string(image.base+value+2)
  imports.append(dict(iat_address=hex(image.base+iat+index*4),dll=dll,symbol=symbol))
targets=[entry for entry in imports if int(entry['iat_address'],16) in (0x100700F0,0x100700F4)]
assert len(targets)==2
lines=[]
for line in Path('work/studiorender-disassembly.txt').read_text(encoding='utf-8').splitlines():
 match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
 if not match:continue
 address=int(match[1],16)
 if not (0x10009B90<=address<0x10009BF0 or 0x10009D7E<=address<0x10009D93):continue
 prefix=bytes.fromhex(match[2]);offset=image.offset(address);assert image.b[offset:offset+len(prefix)]==prefix;lines.append(line)
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),resolved_imports=targets,verified_instruction_prefixes=len(lines),scope='Read-only PE import descriptor resolution and offline instruction prefixes; imported runtime behavior not executed')
Path('outputs/sfm64-flex-exit-import-resolution.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
Path('outputs/sfm64-flex-exit-import-disassembly.txt').write_text('\n'.join(lines)+'\n',encoding='utf-8')
print(json.dumps(targets,indent=2))
