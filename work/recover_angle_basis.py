import hashlib,json,re,struct
from pathlib import Path
from pe_image import Image
image=Image(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll'))
begin,end=0x10498be0,0x10498d51;cursor=begin;spans=[]
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
report={'binary_sha256':hashlib.sha256(image.b).hexdigest(),'routine':'10498be0','verified_instruction_spans':spans,'angle_inputs':'pitch,yaw,roll at offsets0,4,8','angle_scale_word':hex(image.word(0x10a8bd00)),'negative_one_word':hex(image.word(0x10a8a66c)),'sign_mask_words':[hex(image.word(0x10a8f3c0+4*i)) for i in range(4)],'trig':'Each angle multiplies scale in scalar SSE, stores float, FLD then FSINCOS then FSTP cos/sin as floats; yaw then pitch then roll.','outputs':['forward optional arg2','right optional arg3','up optional arg4'],'jitter_inputs':'10723680 passes output pointers stack94 forward, stackac right, stackb8 up; first displacement uses right and jitter.x; second uses up and jitter.y','limits':'Full instruction evidence; no original execution parity yet. Owned arithmetic stage accepts explicitly supplied rounded trig words and does not substitute standard sin/cos for original FSINCOS.'}
Path('outputs/sfm64-angle-basis-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('angle_basis_spans='+str(len(spans)))
