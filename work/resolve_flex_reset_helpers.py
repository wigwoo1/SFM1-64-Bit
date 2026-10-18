from pathlib import Path
from pe_image import Image
import struct,json,hashlib,re
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf\bin\client.dll')
optional=image.u32(60)+24;directory=image.u32(optional+104);size=image.u32(optional+108);imports=[]
for index in range(size//20):
 offset=image.offset(image.base+directory+index*20)
 original,stamp,forwarder,name,iat=struct.unpack_from('<5I',image.b,offset)
 if not any((original,stamp,forwarder,name,iat)):break
 for slot in range(65536):
  value=image.word(image.base+(original or iat)+slot*4)
  if not value:break
  address=image.base+iat+slot*4
  if address!=0x104AE2C4:continue
  imports.append(dict(address=hex(address),dll=image.string(image.base+name),symbol=f'ordinal:{value&0xffff}' if value&0x80000000 else image.string(image.base+value+2)))
assert len(imports)==1
decoded=[]
for line in Path('work/native-client-scale-disassembly.txt').read_text(encoding='utf-8').splitlines():
 match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
 if match:decoded.append([int(match[1],16),bytes.fromhex(match[2]),line.strip()])
 elif decoded:
  tail=re.fullmatch(r'\s+((?:[0-9A-F]{2} ?)+)\s*',line)
  if tail:decoded[-1][1]+=bytes.fromhex(tail[1])
checked=[]
for address,data,line in decoded:
 if not(0x10138010<=address<0x10138047 or 0x1045B750<=address<0x1045B7C9 or 0x10465C0E<=address<0x10465CC8):continue
 offset=image.offset(address);assert image.b[offset:offset+len(data)]==data
 checked.append(dict(address=hex(address),bytes=data.hex(),instruction=line))
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),allocator_import=imports,allocation_wrapper='0x10138010: virtual byte offset 4',release_wrapper='0x10138030: virtual byte offset 0x14',fill_helper='0x1045b750',large_zero_fill='0x10465c0e',verified_full_instructions=checked,limitations='Allocator import, scalar fill and complete large zero-fill routine checked statically. Allocator runtime behavior and actual dispatch not executed.')
Path('outputs/sfm64-flex-reset-helper-resolution.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(imports);print('Verified',len(checked),'complete helper instructions')
