from pathlib import Path
import struct,re,json
binary=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\sdktools\python\2.7\win32\Lib\site-packages\vs\datamodel_pyd.pyd')
data=binary.read_bytes();pe=struct.unpack_from('<I',data,60)[0];base=struct.unpack_from('<I',data,pe+52)[0];count=struct.unpack_from('<H',data,pe+6)[0];optional=struct.unpack_from('<H',data,pe+20)[0];table=pe+24+optional;sections=[]
for n in range(count):
 o=table+n*40;size,rva,rawsize,raw=struct.unpack_from('<IIII',data,o+8);sections.append((raw,rawsize,rva))
def offset(address):
 rva=address-base
 for raw,size,start in sections:
  if start<=rva<start+size:return raw+rva-start
 raise ValueError('address outside section')
constant=struct.unpack_from('<f',data,offset(0x104DECD8))[0]
lines=Path('work/datamodel-pyd-camera-disassembly.txt').read_text().splitlines();callers=[]
for index,line in enumerate(lines):
 if re.search(r'\b(call|jmp)\s+100775C0\b',line):callers.append(dict(instruction=line,context=lines[max(0,index-10):index+10]))
pointer=struct.pack('<I',0x100775C0);matches=[];cursor=0
while True:
 o=data.find(pointer,cursor)
 if o<0:break
 address=next((base+rva+o-raw for raw,size,rva in sections if raw<=o<raw+size),None)
 matches.append(dict(file_offset=o,address=hex(address) if address is not None else None));cursor=o+1
result=dict(binary=str(binary),comparison_constant=constant,direct_callers=callers,function_pointer_candidates=matches,limitations='Pointer matches and direct callers do not by themselves establish class identity or runtime parity.')
Path('outputs/sfm64-native-scaled-transform-callers.json').write_text(json.dumps(result,indent=2)+'\n')
print('Comparison constant:',constant,'direct callers:',len(callers),'pointer candidates:',len(matches))
for c in callers:print(c['instruction'])
assert constant==1.0
