"""Recover the validated push/push/mov/call registration pattern."""
import collections,json,pathlib,re,struct
root=pathlib.Path(__file__).resolve().parent
b=pathlib.Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll').read_bytes()
p=struct.unpack_from('<I',b,60)[0];opt=p+24; sections=[]
for i in range(struct.unpack_from('<H',b,p+6)[0]):
 o=opt+struct.unpack_from('<H',b,p+20)[0]+40*i
 sections.append((struct.unpack_from('<I',b,o+12)[0],struct.unpack_from('<I',b,o+16)[0],struct.unpack_from('<I',b,o+20)[0]))
def string(va):
 for rva,size,raw in sections:
  if rva<=va-0x10000000<rva+size:
   o=raw+va-0x10000000-rva
   return b[o:b.index(b'\0',o)].decode('ascii')
 raise ValueError(hex(va))
recent=collections.deque(maxlen=3);records=[]
with (root/'ifm-disassembly.txt').open() as f:
 for line in f:
  if 'call        10980B80' in line:
   text=''.join(recent)
   pushes=re.findall(r'push\s+([0-9A-F]+)h',text)
   node=re.search(r'mov\s+ecx,([0-9A-F]+)h',text)
   if len(pushes)!=2 or not node: raise ValueError('Unrecognized registration '+text)
   records.append(dict(name=string(int(pushes[0],16)),name_va=pushes[0],factory_va=pushes[1],node_va=node[1],call_va=line.strip().split(':')[0]))
  recent.append(line)
out=root.parent/'outputs'/'sfm-registered-interfaces.json'
out.write_text(json.dumps(dict(method='Static constructor pattern; runtime execution not verified',registrations=records),indent=2))
print(json.dumps(records,indent=2))
