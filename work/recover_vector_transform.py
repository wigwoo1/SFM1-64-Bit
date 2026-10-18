import hashlib,json,re,struct
from pathlib import Path
from pe_image import Image
image=Image(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll'))
begin,end=0x10498390,0x10498431;cursor=begin;spans=[]
with Path('work/ifm-disassembly.txt').open() as source:
 previous=None
 for line in source:
  match=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
  if match:
   if previous:
    address,data=previous;assert address==cursor;at=image.offset(address);assert image.b[at:at+len(data)]==data
    spans.append({'address':hex(address),'bytes':data.hex()});cursor+=len(data);previous=None
   address=int(match[1],16)
   if address>=end:break
   if address>=begin:previous=(address,bytes.fromhex(match[2]))
  elif previous:
   continuation=re.fullmatch(r'\s+((?:[0-9A-F]{2}\s*)+)',line)
   if continuation:previous=(previous[0],previous[1]+bytes.fromhex(continuation[1]))
assert cursor==end,(hex(cursor),hex(end))
report={'binary_sha256':hashlib.sha256(image.b).hexdigest(),'routine':'10498390','verified_instruction_spans':spans,'operation':'For each matrix row: matrix.y*position.y + matrix.x*position.x, then matrix.z*position.z, then row translation. Scalar SSE sequential products/adds.','arguments':['position pointer','matrix float3x4 pointer','output float3 pointer'],'limits':'World frame lookup remains unverified; nonaliasing owned API and default FP environment only.'}
Path('outputs/sfm64-vector-transform-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('vector_transform_spans='+str(len(spans)))
