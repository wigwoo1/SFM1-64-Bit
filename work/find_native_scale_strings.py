from pathlib import Path
import json,struct
game=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
paths=sorted(set(list(game.glob('bin/*.dll'))+list(game.glob('*/bin/*.dll'))+list((game/'sdktools/python').rglob('*.pyd'))))
records=[]
for path in paths:
 data=path.read_bytes()
 if len(data)<64 or data[:2]!=b'MZ':continue
 pe=struct.unpack_from('<I',data,60)[0]
 if pe+24>len(data) or data[pe:pe+4]!=b'PE\0\0':continue
 sections=struct.unpack_from('<H',data,pe+6)[0];optional=struct.unpack_from('<H',data,pe+20)[0];magic=struct.unpack_from('<H',data,pe+24)[0]
 base=struct.unpack_from('<I' if magic==0x10b else '<Q',data,pe+24+(28 if magic==0x10b else 24))[0]
 table=pe+24+optional;spans=[]
 for n in range(sections):
  o=table+n*40
  if o+40>len(data):break
  size,rva,rawsize,raw=struct.unpack_from('<IIII',data,o+8);spans.append((raw,rawsize,rva))
 for needle in (b'\0scale\0',b'\0boneScale\0',b'\0boneTransforms\0'):
  start=0
  while True:
   offset=data.find(needle,start)
   if offset<0:break
   offset+=1;start=offset+len(needle)-1
   rva=next((rva+offset-raw for raw,size,rva in spans if raw<=offset<raw+size),None)
   refs=[]
   if rva is not None and magic==0x10b:
    address=struct.pack('<I',base+rva);cursor=0
    while True:
     ref=data.find(address,cursor)
     if ref<0:break
     refs.append(ref);cursor=ref+1
   records.append(dict(binary=str(path),string=needle[1:-1].decode(),file_offset=offset,rva=rva,address=base+rva if rva is not None else None,absolute_address_byte_matches=refs))
Path('outputs/sfm64-native-scale-string-locations.json').write_text(json.dumps(dict(scope='Read-only PE string and absolute-address byte scan; matches are candidates, not proven code references',locations=records),indent=2)+'\n')
print('PE binaries scanned:',len(paths),'candidate strings:',len(records))
for record in records:print(Path(record['binary']).name,record['string'],hex(record['address'] or 0),'address_matches',len(record['absolute_address_byte_matches']))
