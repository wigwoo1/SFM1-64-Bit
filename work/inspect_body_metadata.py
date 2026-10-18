from pathlib import Path
import struct,json
b=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\anno\mamagen\mamagen.mdl').read_bytes();count,start=struct.unpack_from('<ii',b,232)
parts=[]
for n in range(count):
 at=start+n*16;name,models,base,offset=struct.unpack_from('<iiii',b,at);end=b.index(0,at+name);parts.append({'name':b[at+name:end].decode(),'model_count':models,'base':base})
Path('outputs/sfm64-live-model-body-metadata.json').write_text(json.dumps(parts,indent=2));print(parts)
