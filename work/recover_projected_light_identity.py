import hashlib,json,struct
from pathlib import Path
from pe_image import Image
image=Image(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll'))
def va(offset):
 for rva,size,raw in image.sections:
  if raw<=offset<raw+size:return image.base+rva+offset-raw
 raise ValueError('unmapped file offset')
def matches(pattern):
 at=0;result=[]
 while True:
  at=image.b.find(pattern,at)
  if at<0:break
  result.append(at);at+=1
 return result
name=b'.?AVCDmeProjectedLight@@\0';found=matches(name);assert len(found)==1
typeAddress=va(found[0])-8;candidates=[]
for offset in matches(struct.pack('<I',typeAddress)):
 if offset<12:continue
 colOffset=offset-12
 if image.u32(colOffset)!=0:continue
 colAddress=va(colOffset)
 for tableOffset in matches(struct.pack('<I',colAddress)):
  table=va(tableOffset)+4
  try:entries=[image.word(table+4*n) for n in range(16)]
  except ValueError:continue
  if all(image.base<=entry<image.base+0x1000000 for entry in entries):
   candidates.append({'locator':hex(colAddress),'subobject_offset':image.u32(colOffset+4),'vtable':hex(table),'first16_targets':[hex(entry) for entry in entries]})
fields={}
for field in ['DmeProjectedLight','intensity','horizontalFOV','verticalFOV','constantAttenuation','quadraticAttenuation','shadowDepthBias','ambientIntensity','uberlight']:
 fields[field]=[hex(va(offset)) for offset in matches(field.encode()+b'\0')]
report={'binary_sha256':hashlib.sha256(image.b).hexdigest(),'type_descriptor':hex(typeAddress),'vtable_candidates':candidates,'field_string_addresses':fields,'limits':'RTTI/string identity and structural candidates only. Vtable slot semantics, projected-light conversion, live field evaluation and renderer parity remain unverified. String references may be shared by other classes.'}
Path('outputs/sfm64-projected-light-identity-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('projected_light_type_descriptor='+hex(typeAddress)+' vtable_candidates='+str(len(candidates)))
