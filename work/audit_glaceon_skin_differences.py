from pathlib import Path
import struct,json
p=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\glaceon.mdl');b=p.read_bytes()
def u(o):return struct.unpack_from('<I',b,o)[0]
def text(o):return b[o:b.index(0,o)].decode(errors='replace')
names=[text(u(208)+n*64+struct.unpack_from('<i',b,u(208)+n*64)[0]) for n in range(u(204))]
slots,families,start=struct.unpack_from('<iii',b,220);skins=[list(struct.unpack_from('<'+'H'*slots,b,start+f*slots*2)) for f in range(families)]
changes=[{'slot':n,'material_names':[names[s[n]] for s in skins]} for n in range(slots) if len({s[n] for s in skins})>1]
count,offset=struct.unpack_from('<ii',b,232);uses=[]
for group in range(count):
 at=offset+group*16;models,base,mo=struct.unpack_from('<iii',b,at+4)
 for part in range(models):
  model=at+mo+part*148;meshCount,meshOffset=struct.unpack_from('<ii',b,model+72)
  for mesh in range(meshCount):
   slot=struct.unpack_from('<i',b,model+meshOffset+mesh*116)[0]
   if any(c['slot']==slot for c in changes):uses.append({'body_group':group,'part':part,'mesh':mesh,'slot':slot,'body_base':base})
report={'skin_families':families,'changed_material_slots':changes,'mesh_uses':uses};Path('outputs/sfm64-glaceon-skin-difference-audit.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
