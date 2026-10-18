import hashlib,json,re,struct
from pathlib import Path
from pe_image import Image
image=Image(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll'))
begin,end=0x1049d180,0x1049d347;cursor=begin;spans=[]
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
report={'binary_sha256':hashlib.sha256(image.b).hexdigest(),'routine':'1049d180','verified_instruction_spans':spans,'trace_threshold_word':hex(image.word(0x10b36660)),'norm_comparison_word':hex(image.word(0x10bf5c50)),'branching':'Trace+1 COMISS/JBE selects trace or dominant diagonal quaternion branch; unordered follows JBE. Quaternion squared norm computes x*x+y*y+z*z+w*w sequentially, then UCOMISS/LAHF/TEST44/JNP skips normalization only on ordered equality with comparison constant; unordered normalizes.','normalization':'SQRTSS, one/norm DIVSS, then multiplies w,z,y,x in that order','translation':'x87 FLD/FSTP translation column offsets0xc/0x1c/0x2c','limits':'Instruction verification only; orientation execution parity and world renderer integration remain unverified.'}
Path('outputs/sfm64-matrix-orientation-extraction-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('matrix_orientation_spans='+str(len(spans))+' norm_compare='+report['norm_comparison_word'])
