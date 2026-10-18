from pathlib import Path
import json,hashlib,struct
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
audit=json.loads(Path('outputs/sfm64-color-generator-dependencies.json').read_text())
assert audit['sha256']==hashlib.sha256(im.b).hexdigest()
# Preserve16-byte alignment and within-region relative addresses. Code and data
# have separate page ranges so the harness can protect them RX and R.
regions=[(0x100e0000,0x2000,0),(0x10153000,0x7000,0x4000)]
image=bytearray(0xb000)
for original,size,destination in regions:
 offset=im.offset(original)
 assert im.offset(original+size-1)==offset+size-1
 image[destination:destination+size]=im.b[offset:offset+size]
def translated(address):
 for original,size,destination in regions:
  if original<=address<original+size:return destination+address-original
 raise ValueError(hex(address))
fixups=[];seen=set()
for result in audit['reachable_control_flow'].values():
 assert not result['unresolved_edges'] and not result['traversal_limit_hit']
 assert not result['direct_and_indirect_calls']
 for relocation in result['absolute_relocations']:
  address=int(relocation['fixup'],16);target=int(relocation['target'],16)
  assert relocation['target_file_backed']
  offset=translated(address);target_offset=translated(target)
  assert struct.unpack_from('<I',image,offset)[0]==target
  if address not in seen:fixups.append({'offset':offset,'target_offset':target_offset});seen.add(address)
manifest={'installed_sha256':audit['sha256'],'image_bytes':len(image),'image_sha256':hashlib.sha256(image).hexdigest(),'regions':[{'original':hex(a),'size':s,'offset':o} for a,s,o in regions],'fixups':fixups,'entries':{'double':translated(0x100e01aa),'float':translated(0x100e1154)},'limits':'Preparation only; no native execution. Data region conservatively includes referenced constant/table ranges, but indexed access bounds and writable accesses require harness/runtime validation. Extra code pages contain neighboring bytes that are not declared reachable. Only verified reachable-instruction relocations are remapped; never invoke other entries.'}
Path('work/color-math-native-image.bin').write_bytes(image)
Path('outputs/sfm64-color-math-native-image.json').write_text(json.dumps(manifest,indent=2)+'\n')
header='#pragma once\nstatic constexpr unsigned colorMathFixups[][2]={'+','.join('{'+str(f['offset'])+','+str(f['target_offset'])+'}' for f in fixups)+'};\n'
Path('work/color_math_native_fixups.h').write_text(header)
print('prepared_image_bytes='+str(len(image))+' reachable_fixups='+str(len(fixups)))
