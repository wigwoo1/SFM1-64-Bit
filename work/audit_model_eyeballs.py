from pathlib import Path
import struct,json
records=[]
for name in ['glaceon','lotion cat']:
 b=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models',name+'.mdl').read_bytes();count,start=struct.unpack_from('<ii',b,232)
 for body in range(count):
  at=start+body*16;models,offset=struct.unpack_from('<i8xi',b,at+4)
  # Body part model offset is at +12; count at +4.
  models=struct.unpack_from('<i',b,at+4)[0];offset=struct.unpack_from('<i',b,at+12)[0]
  for model in range(models):
   m=at+offset+model*148;eyes,eo=struct.unpack_from('<ii',b,m+100)
   for eye in range(eyes):
    e=m+eo+eye*172;relative,bone=struct.unpack_from('<ii',b,e);end=b.index(0,e+relative)
    records.append({'asset':name,'body':body,'model':model,'eye':eye,'name':b[e+relative:end].decode(),'bone':bone,'origin':struct.unpack_from('<3f',b,e+8),'zoffset':struct.unpack_from('<f',b,e+20)[0],'radius':struct.unpack_from('<f',b,e+24)[0],'up':struct.unpack_from('<3f',b,e+28),'forward':struct.unpack_from('<3f',b,e+40),'material_slot':struct.unpack_from('<i',b,e+52)[0],'iris_scale':struct.unpack_from('<f',b,e+60)[0]})
Path('outputs/sfm64-model-eyeball-metadata-audit.json').write_text(json.dumps(records,indent=2)+'\n');print(json.dumps(records,indent=2))
