import hashlib,json,re,struct
from pathlib import Path
from pe_image import Image
image=Image(Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll'))
begin,end=0x105c1430,0x105c1762;cursor=begin;spans=[]
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
fields=[]
for offset,nameAddress,defaultAddress,destination in [(0xa8,0x10ae622c,0x10b94314,None),(0xb0,0x10ae6278,None,0x48),(0xb8,0x10ae6264,None,0x44),(0xc0,0x10ae624c,0x10b94318,0x40),(0xc8,0x10ae628c,0x10aaa43c,0x20)]:
 bits=image.word(defaultAddress) if defaultAddress else 0
 fields.append({'name':image.string(nameAddress),'cached_value_offset':hex(offset),'attribute_pointer_offset':hex(offset+4),'type':3,'default_word':hex(bits),'default_float':struct.unpack('<f',struct.pack('<I',bits))[0],'projected_output_offset':hex(destination) if destination is not None else None})
report={'binary_sha256':hashlib.sha256(image.b).hexdigest(),'verified_instruction_spans':spans,'registration_routines':['105c1430','105c16a0'],'lifecycle_linkage':'Both preceding calls are verified in projected-light slot10 entry10723d90','fields':fields,'color':{'name':image.string(0x10a8c7ec),'cached_value_offset':'0xa0','attribute_pointer_offset':'0xa4','type':8,'default_word':'0xffffffff','setter':'1036cd90'},'limits':'Instruction recovery, not runtime execution parity. Ordinary-light RGB routine105c14c0 multiplies intensity by scale first, unlike projected routine10723680 which multiplies channel by intensity first; do not merge operation order. Cached values require live evaluation before conversion.'}
Path('outputs/sfm64-inherited-light-field-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('inherited_light_fields='+str([(f['name'],f['default_float']) for f in fields]))
