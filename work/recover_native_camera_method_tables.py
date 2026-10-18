from pathlib import Path
from pe_image import Image
import struct,json,hashlib
root=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\sdktools\python\2.7\win32\Lib\site-packages\vs')
records=[]
for filename,names in [('valve_pyd.pyd',['ComputeProjectionMatrix','MatrixBuildPerspective']),('datamodel_pyd.pyd',['CDmeCamera_GetProjectionMatrix','CDmeCamera_GetViewMatrix','CDmeCamera_GetFOVx'])]:
 im=Image(root/filename)
 def address(offset):
  for rva,size,raw in im.sections:
   if raw<=offset<raw+size:return im.base+rva+offset-raw
  raise ValueError('no mapped section')
 for name in names:
  needle=name.encode()+b'\0';start=0
  while True:
   at=im.b.find(needle,start)
   if at<0:break
   start=at+1
   if at and im.b[at-1]!=0:continue
   va=address(at);pointer=struct.pack('<I',va);scan=0
   while True:
    ref=im.b.find(pointer,scan)
    if ref<0:break
    scan=ref+1
    if ref+16>len(im.b):continue
    fn,flags,doc=struct.unpack_from('<III',im.b,ref+4)
    if flags not in (1,2,3,4,8):continue
    try:code=im.offset(fn)
    except ValueError:continue
    record={'binary':filename,'sha256':hashlib.sha256(im.b).hexdigest(),'name':name,'name_va':hex(va),'table_va':hex(address(ref)),'wrapper_va':hex(fn),'flags':flags,'wrapper_first_32_bytes':im.b[code:code+32].hex()}
    records.append(record);print(name,hex(fn),'table',hex(address(ref)))
Path('outputs/sfm64-native-camera-method-addresses.json').write_text(json.dumps(records,indent=2))
if len(records)<5:raise RuntimeError('expected camera method-table entries missing')
