from pe_image import Image
from pathlib import Path
import struct,json,hashlib
path=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\sdktools\python\2.7\win32\Lib\site-packages\vs\datamodel_pyd.pyd')
pe=Image(path)
def va(offset):
 for rva,size,raw in pe.sections:
  if raw<=offset<raw+size:return pe.base+rva+offset-raw
 raise ValueError('unmapped offset')
records=[]
for name in [b'CDmeTransform_GetTransform',b'CDmeDag_GetLocalTransform',b'CDmeDag_GetTransform']:
 cursor=0
 while True:
  offset=pe.b.find(name+b'\0',cursor)
  if offset<0:break
  address=va(offset);pointer=struct.pack('<I',address);refs=[];start=0
  while True:
   found=pe.b.find(pointer,start)
   if found<0:break
   refs.append({'address':hex(va(found)),'following_word':hex(pe.u32(found+4))});start=found+1
  records.append({'name':name.decode(),'string_address':hex(address),'references':refs});cursor=offset+1
report={'binary':str(path),'sha256':hashlib.sha256(pe.b).hexdigest(),'bindings':records,'scale_name':pe.string(0x104e53c8),'scale_one':struct.unpack_from('<f',pe.b,pe.offset(0x104decd8))[0],'limits':'String/pointer recovery only. Wrapper code and live behavior require further verification.'}
Path('outputs/sfm64-transform-scale-binding-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
assert pe.b[pe.offset(0x10293aef):pe.offset(0x10293aef)+5]==bytes.fromhex("e8 cc 3a de ff")
report["verified_wrapper_call"]={"wrapper":"0x10293a10","call":"0x10293aef","target":"0x100775c0","bytes":"e8 cc 3a de ff"}
report["limits"]="Installed SWIG wrapper identifies CDmeTransform_GetTransform and calls the scale-aware routine. Full native scene rendering parity remains unverified."
Path("outputs/sfm64-transform-scale-binding-recovery.json").write_text(json.dumps(report,indent=2)+"\n")
