import hashlib,json,re,struct
from pathlib import Path
from pe_image import Image
image=Image(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll'))
begin,end=0x1049dfc0,0x1049e155;cursor=begin;spans=[]
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
report={'binary_sha256':hashlib.sha256(image.b).hexdigest(),'routine':'1049dfc0','verified_instruction_spans':spans,'arguments':['quaternion pointer','position pointer','output float3x4 pointer'],'constants':{'two':hex(image.word(0x10a8baa4)),'one':hex(image.word(0x10a8a56c))},'rotation':'Quaternion x,y,z,w; each doubled product is (component*2)*component, then sequential ADDSS/SUBSS; no normalization','translation':'x87 FLD/FSTP position to output offsets0xc/0x1c/0x2c','limits':'Instruction-derived convention, not execution parity. Aliasing, floating-point environment and original NaN operand propagation remain to validate.'}
Path('outputs/sfm64-quaternion-position-matrix-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('quaternion_matrix_spans='+str(len(spans)))
