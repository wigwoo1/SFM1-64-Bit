from pathlib import Path
import re,struct,json
lines=Path('work/viewport-procedural-disassembly.txt').read_text().splitlines();selected=[]
for line in lines:
 m=re.match(r'\s*([0-9A-F]{8}):',line)
 if m and 0x10196B80<=int(m[1],16)<0x10196BE0:selected.append(line)
data=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\sdktools\python\2.7\win32\Lib\site-packages\vp.pyd').read_bytes();pe=struct.unpack_from('<I',data,60)[0];count=struct.unpack_from('<H',data,pe+6)[0];optional=struct.unpack_from('<H',data,pe+20)[0];base=struct.unpack_from('<I',data,pe+24+28)[0];spans=[]
for n in range(count):
 o=pe+24+optional+n*40;virtual,rva,size,raw=struct.unpack_from('<IIII',data,o+8);spans.append((rva,size,raw))
strings={}
for address in (0x10248D7C,0x10248D68,0x10248D2C,0x10248D10):
 rva=address-base;offset=next(raw+rva-start for start,size,raw in spans if start<=rva<start+size);end=data.index(0,offset);strings[hex(address)]=data[offset:end].decode('ascii',errors='replace')
text='\n'.join(selected)+'\n'+json.dumps(strings,indent=2)+'\n';Path('outputs/sfm64-viewport-procedural-branch-target.txt').write_text(text);print(text)
