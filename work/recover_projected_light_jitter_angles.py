import hashlib,json,re,struct
from pathlib import Path
from pe_image import Image
image=Image(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll'))
begin,end=0x1049e160,0x1049e353;cursor=begin;spans=[]
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
report={'binary_sha256':hashlib.sha256(image.b).hexdigest(),'routine':'1049e160','verified_instruction_spans':spans,'input':'quaternion x,y,z,w','output':'three angle words at offsets0,4,8','horizontal_norm':'SQRTSS of two quaternion-derived rotation terms squared, with sequential sums','singularity_threshold_word':hex(image.word(0x10a8e010)),'angle_scale_word':hex(image.word(0x10a8bd04)),'angle_math_helper':'10a0e901 receives two converted doubles in xmm0/xmm1, returns xmm0; implementation not fully recovered','normal_branch':'three helper calls; each double result converts to float then multiplies angle scale','singular_branch':'two helper calls and positive-zero third angle','subsequent_basis_helper':'10498be0 uses three original x87 FSINCOS operations after float multiplication by constant10a8bd00','basis_radian_scale_word':hex(image.word(0x10a8bd00)),'jitter_order':'10723680 calls quaternion->angles then angle->basis; first displacement multiplies component by jitter.x then radius, adds to origin; second displacement multiplies component by jitter.y then radius and adds to first displaced origin','limits':'Instruction evidence for full quaternion-angle wrapper only. Angle math helper, full basis routine, FP singularities and direct original jitter execution parity remain unverified. Do not replace path with direct quaternion matrix columns without parity evidence.'}
Path('outputs/sfm64-projected-light-jitter-angle-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('jitter_angle_spans='+str(len(spans)))
