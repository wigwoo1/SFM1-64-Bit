from pathlib import Path
import struct,json,collections,math
path=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\sneakyerz\wigwoo1\hobkin\hobkin_3_jigglebones.mdl');data=path.read_bytes()
count,start=struct.unpack_from('<ii',data,156);records=[]
for index in range(count):
 off=start+index*216
 if off<0 or off+216>len(data):raise ValueError('bone span')
 kind,relative=struct.unpack_from('<ii',data,off+164)
 if not kind:continue
 nameoff=off+struct.unpack_from('<i',data,off)[0];end=data.index(0,nameoff);name=data[nameoff:end].decode(errors='replace')
 proc=off+relative
 if not 0<=proc<len(data):raise ValueError('procedure offset')
 flags=struct.unpack_from('<I',data,proc)[0]
 fields='length tipMass yawStiffness yawDamping pitchStiffness pitchDamping alongStiffness alongDamping angleLimit minYaw maxYaw yawFriction yawBounce minPitch maxPitch pitchFriction pitchBounce baseMass baseStiffness baseDamping baseMinLeft baseMaxLeft baseLeftFriction baseMinUp baseMaxUp baseUpFriction baseMinForward baseMaxForward baseForwardFriction'.split()
 if kind!=5 or proc+120>len(data):raise ValueError('unsupported procedural record')
 values=struct.unpack_from('<29f',data,proc+4)
 if not all(math.isfinite(v) for v in values):raise ValueError('nonfinite jiggle parameters')
 active=[label for bit,label in [(1,'flexible'),(2,'rigid'),(4,'yaw_constraint'),(8,'pitch_constraint'),(16,'angle_constraint'),(32,'length_constraint'),(64,'base_spring'),(128,'boing')] if flags&bit]
 records.append(dict(index=index,name=name,parent=struct.unpack_from('<i',data,off+4)[0],procedure_type=kind,procedure_file_offset=proc,flags=flags,active_features=active,parameters=dict(zip(fields,values)),record_interpretation='120-byte SFM candidate prefix; SDK boing extension not read'))
result=dict(model=str(path),bone_count=count,procedural_bones=records,type_counts=dict(collections.Counter(r['procedure_type'] for r in records)),evaluation='not implemented',interpretation='Type identity must be checked against SDK and installed native behavior')
Path('outputs/sfm64-scene-procedural-bones.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
