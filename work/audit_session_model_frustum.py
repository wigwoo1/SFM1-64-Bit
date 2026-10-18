from pathlib import Path
import struct,json
camera=struct.unpack('<16f',Path('work/session-camera-engine-view-v1.bin').read_bytes())
model=struct.unpack('<16f',Path('work/session-mamagen-placement-v1.bin').read_bytes())
vvd=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\anno\mamagen\mamagen.vvd').read_bytes()
count=struct.unpack_from('<I',vvd,16)[0];start=struct.unpack_from('<I',vvd,56)[0]
def project(matrix,p):return [sum(matrix[r*4+c]*p[c] for c in range(4)) for r in range(4)]
visible=0;front=0;bounds=[[float('inf'),float('-inf')] for _ in range(3)]
for n in range(count):
 p=list(struct.unpack_from('<3f',vvd,start+n*48+16))+[1]
 clip=project(camera,project(model,p))
 if clip[3]<=0:continue
 front+=1;ndc=[clip[c]/clip[3] for c in range(3)]
 for c in range(3):bounds[c][0]=min(bounds[c][0],ndc[c]);bounds[c][1]=max(bounds[c][1],ndc[c])
 visible+=(-1<=ndc[0]<=1 and -1<=ndc[1]<=1 and 0<=ndc[2]<=1)
result={'vertices':count,'front_of_eye':front,'inside_frustum':visible,'ndc_bounds':bounds,'scope':'Raw VVD bind-pose positions under candidate saved model and engine-camera matrices; session bone pose is not applied.'}
Path('outputs/sfm64-session-model-frustum-audit.json').write_text(json.dumps(result,indent=2));print(result)
