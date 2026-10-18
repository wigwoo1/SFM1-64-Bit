from pathlib import Path
import struct,re,json
mount=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod');records=[]
for model in ['glaceon','lotion cat']:
 b=(mount/'models'/f'{model}.mdl').read_bytes()
 def word(o):return struct.unpack_from('<I',b,o)[0]
 def text(o):return b[o:b.index(0,o)].decode(errors='replace')
 names=[text(word(208)+n*64+struct.unpack_from('<i',b,word(208)+n*64)[0]) for n in range(word(204))]
 dirs=[text(word(word(216)+n*4)) for n in range(word(212))]
 for name in names:
  for directory in dirs:
   vmt=mount/'materials'/(directory.replace('\\','/')+name+'.vmt')
   if not vmt.exists():continue
   source=vmt.read_text(errors='replace');m=re.search(r'\$basetexture"?\s+"?([^"\s]+)',source,re.I)
   if not m:continue
   vtf=(mount/'materials'/m[1].replace('\\','/')).with_suffix('.vtf')
   if not vtf.exists():continue
   with vtf.open('rb') as f:h=f.read(128)
   version=struct.unpack_from('<II',h,4);header=struct.unpack_from('<I',h,12)[0]
   record={'model':model,'texture':str(vtf),'version':version,'header_size':header,'file_size':vtf.stat().st_size,'format':struct.unpack_from('<I',h,52)[0],'resource_count':struct.unpack_from('<I',h,68)[0] if version[1]>=3 else None,'first_128_bytes':h.hex()}
   records.append(record);break
Path('outputs/sfm64-failing-texture-header-audit.json').write_text(json.dumps(records,indent=2)+'\n')
print(json.dumps([{k:v for k,v in r.items() if k!='first_128_bytes'} for r in records],indent=2))
