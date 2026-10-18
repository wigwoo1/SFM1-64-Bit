import hashlib,json,re,struct
from pathlib import Path
from pe_image import Image
image=Image(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll'))
begin,end=0x104d2860,0x104d2938;cursor=begin;spans=[]
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
report={'binary_sha256':hashlib.sha256(image.b).hexdigest(),'routine':'104d2860','verified_instruction_spans':spans,'matrix_builder':'1049dfc0 with pointers this+0x6c,this+0x5c,output','optional_scale':{'name':image.string(0x10aa7864),'lookup':'1035dd50','required_type':3,'accessor':'100276d0','comparison_constant_word':hex(image.word(0x10a8a56c)),'branch':'UCOMISS scale,one; LAHF; TEST AH,44h; JNP skips multiplication only when ordered equal; unordered multiplies','multiply_output_offsets':['0x0','0x4','0x8','0x10','0x14','0x18','0x20','0x24','0x28'],'translation_offsets_untouched':['0xc','0x1c','0x2c']},'limits':'Offline byte verification only. Builder input field identities and quaternion operation order still need recovery. Optional scale is absent from retained light transform audit and must not be silently discarded on other sessions.'}
Path('outputs/sfm64-transform-local-matrix-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('local_matrix_spans='+str(len(spans))+' optional_scale='+report['optional_scale']['name'])
