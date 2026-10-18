from pathlib import Path
import json,struct,hashlib
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
base=json.loads(Path('outputs/sfm64-color-math-native-image.json').read_text())
audit=json.loads(Path('outputs/sfm64-color-generator-dependencies.json').read_text())
assert base['installed_sha256']==audit['sha256']==hashlib.sha256(im.b).hexdigest()
image=bytearray(Path('work/color-math-native-image.bin').read_bytes())
assert hashlib.sha256(image).hexdigest()==base['image_sha256']
regions=[(int(r['original'],16),r['size'],r['offset']) for r in base['regions']]
regions.extend([(0x100d5000,0x1000,0xc000),(0x10190000,0x9000,0x10000)])
image.extend(bytes(0x19000-len(image)))
source=im.offset(0x100d5000);image[0xc000:0xd000]=im.b[source:source+0x1000]
# Outputs are private zero-initialized storage, not original runtime globals.
fixups=list(base['fixups']);generator_fixups=[]
optional=im.u32(60)+24;rva,size=struct.unpack_from('<II',im.b,optional+96+40)
cursor=im.offset(im.base+rva);end=cursor+size
while cursor<end:
 page,length=struct.unpack_from('<II',im.b,cursor)
 if not length:break
 for p in range(cursor+8,cursor+length,2):
  value=im.u16(p);address=im.base+page+(value&4095)
  if value>>12==3 and 0x100d5020<=address<0x100d5368:
   generator_fixups.append((address,im.word(address)))
 cursor+=length
def translate(address):
 for original,size,destination in regions:
  if original<=address<original+size:return destination+address-original
 raise ValueError(hex(address))
for _,target in generator_fixups:
 try:translate(target)
 except ValueError:
  original=target&~4095;destination=len(image)
  source=im.offset(original);assert im.offset(original+4095)==source+4095
  image.extend(im.b[source:source+4096]);regions.append((original,4096,destination))
for address,target in generator_fixups:
 offset=translate(address);assert struct.unpack_from('<I',image,offset)[0]==target
 fixups.append({'offset':offset,'target_offset':translate(target)})
calls=[]
for target,addresses in audit['regions']['generator']['direct_calls'].items():
 for text in addresses:
  address=int(text,16);offset=translate(address);assert image[offset]==0xe8
  assert address+5+struct.unpack_from('<i',image,offset+1)[0]==int(target,16)
  struct.pack_into('<i',image,offset+1,translate(int(target,16))-offset-5)
  calls.append({'original':hex(address),'target':target})
assert len(calls)==7
manifest={'installed_sha256':audit['sha256'],'image_sha256':hashlib.sha256(image).hexdigest(),'image_bytes':len(image),'regions':[{'original':hex(a),'size':s,'offset':o} for a,s,o in regions],'fixups':fixups,'patched_relative_calls':calls,'generator_entry':translate(0x100d5020),'lookup_output':translate(0x101941d0),'limits':'Preparation only. No generator execution. Output region is private zeroed storage. Code-page padding contains unverified neighboring routines and must not be invoked. Native runtime output bounds/control-state and full generator parity remain unverified.'}
Path('work/color-generator-native-image.bin').write_bytes(image)
Path('outputs/sfm64-color-generator-native-image.json').write_text(json.dumps(manifest,indent=2)+'\n')
header='#pragma once\nstatic constexpr unsigned colorGeneratorFixups[][2]={'+','.join('{'+str(f['offset'])+','+str(f['target_offset'])+'}' for f in fixups)+'};\n'
header+='static constexpr unsigned colorGeneratorImageSize='+str(len(image))+';\n'
header+='static constexpr unsigned colorGeneratorLookupOffset='+str(manifest['lookup_output'])+';\n'
digest=1469598103934665603
for byte in image:digest=((digest^byte)*1099511628211)&0xffffffffffffffff
header+='static constexpr unsigned long long colorGeneratorImageDigest='+str(digest)+'ull;\n'
header+='static constexpr unsigned colorGeneratorGammaBits='+str(im.word(0x100fdb70))+';\n'
Path('work/color_generator_native_fixups.h').write_text(header)
print('generator_image_bytes='+str(len(image))+' absolute_fixups='+str(len(fixups))+' relative_calls='+str(len(calls)))
