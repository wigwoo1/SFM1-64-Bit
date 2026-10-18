from pathlib import Path
import hashlib,json,re,struct
from pe_image import Image
path=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\stdshader_dx9.dll');image=Image(path)
lines=Path('work/stdshader_dx9-disassembly.txt').read_text().splitlines()
verified=[]
def span(address,hexbytes):
 data=bytes.fromhex(hexbytes);at=image.offset(address)
 assert image.b[at:at+len(data)]==data,hex(address)
 verified.append({'address':hex(address),'bytes':data.hex()})
def function(begin,end):
 cursor=begin
 for line in lines:
  match=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
  if match and begin<=int(match[1],16)<end:
   address=int(match[1],16);data=bytes.fromhex(match[2]);assert address==cursor
   span(address,data.hex());cursor+=len(data)
 assert cursor==end,(hex(cursor),hex(end))
assert image.string(0x100bbf4c)=='.?AVCShader@UnlitGeneric@@'
assert image.word(image.word(0x1009a778)+12)==0x100bbf44
assert image.word(0x1009a77c+14*4)==0x1004cb30
span(0x1004cbcc,'e8 4f dc 00 00')
span(0x1005c8c2,'e8 39 87 ff ff')
span(0x1005618b,'e8 e0 d9 ff ff')
span(0x10056190,'8b 8d 80 fb ff ff 50 8b 43 44 68 cc cc 09 10 ff d0')
assert image.string(0x1009cccc)=='vertexlit_and_unlit_generic_vs20'
function(0x10053b70,0x10053bbe)
function(0x10053c00,0x10053c1e)
span(0x10059c85,'e8 76 9f ff ff')
span(0x10059c6f,'c7 00 0c 00 00 00')
span(0x10059c8a,'89 06')
fieldData=[(0,0x10055f01,'89 55 b0',0x1008e66c,'VERTEXCOLOR'),
 (4,0x10055f4d,'89 75 b4',0x10094360,'CUBEMAP'),
 (8,0x10055f85,'89 55 b8',0x10090fa4,'HALFLAMBERT'),
 (12,0x10055fba,'89 45 bc',0x1008c1dc,'FLASHLIGHT'),
 (16,0x10055ff3,'89 45 c0',0x1009cd9c,'SEAMLESS_BASE'),
 (20,0x1005602f,'89 45 c4',0x1009cd7c,'SEAMLESS_DETAIL'),
 (24,0x10056084,'89 55 c8',0x1009cd38,'SEPARATE_DETAIL_UVS'),
 (28,0x100560cd,'89 55 cc',0x1009cd10,'STATICLIGHT3'),
 (32,0x100560fe,'89 4d d0',0x1008e63c,'LIGHTING_PREVIEW'),
 (36,0x10056135,'89 45 d4',0x1008f5a0,'TREESWAY'),
 (40,0x10056160,'89 45 d8',0x1008d0e8,'USE_STATIC_CONTROL_FLOW')]
fields=[]
for offset,address,code,nameAddress,name in fieldData:
 span(address,code);assert image.string(nameAddress)==name
 fields.append({'offset':offset,'nearby_debug_label':name,'store':hex(address),'label_address':hex(nameAddress)})
cache=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\platform\shaders\fxc\vertexlit_and_unlit_generic_vs20.vcs')
with cache.open('rb') as file:header=file.read(28)
version,total,dynamic=struct.unpack_from('<III',header);assert version==6 and dynamic==48
report={'binary':str(path),'sha256':hashlib.sha256(image.b).hexdigest(),'unlit_class_draw':'0x1004cb30','shared_draw_helper':'0x1005a820','shared_non_bump_helper_call':'0x1005c8c2 -> 0x10055000','static_packer':'0x10053b70','static_word_offsets':list(range(0,52,4)),'static_scale':48,'cache':str(cache),'cache_dynamic_count':dynamic,'static_selector':'shadow vtable byte offset 0x44 (slot17), name vertexlit_and_unlit_generic_vs20, argument from static packer','dynamic_packer':'0x10053c00','dynamic_word_offsets':list(range(0,24,4)),'dynamic_output':'stored after command word12 at 0x10059c6f; command consumer not yet traced','static_field_labels':fields,'verified_spans':verified,'limits':'Static instruction and class/helper/cache evidence. Full branch conditions, legal field domains, final two static field identities, dynamic field meanings/consumer and native instruction oracle remain unverified. Legacy unlitgeneric diagnostic choice is not the complete installed UnlitGeneric material path.'}
Path('outputs/sfm64-generic-shader-combo-packing-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('generic_combo_instruction_spans='+str(len(verified))+' native_cache_dynamic_count='+str(dynamic))
