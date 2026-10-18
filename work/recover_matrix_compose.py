import hashlib,json,re,struct
from pathlib import Path
from pe_image import Image
image=Image(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll'))
begin,end=0x10498a60,0x10498b35;cursor=begin;spans=[]
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
report={'binary_sha256':hashlib.sha256(image.b).hexdigest(),'routine':'10498a60','verified_instruction_spans':spans,'mask_words':[hex(image.word(0x10b84760+4*i)) for i in range(4)],'operation':'Per left row: (left.z * right.row2 + left.y * right.row1) + left.x * right.row0, then add left row masked to translation lane. All MULPS/ADDPS sequential; no fused operations.','limits':'World frame lookup and separate translation branch remain unresolved; FP modes and aliasing not covered by owned array return API.'}
Path('outputs/sfm64-matrix-compose-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('matrix_compose_spans='+str(len(spans)))
