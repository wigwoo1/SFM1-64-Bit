"""Check installed constructor instructions and adjacent dynamic-field labels."""
import hashlib,json,re
from pathlib import Path
from pe_image import Image

path=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\stdshader_dx9.dll')
image=Image(path)
lines=Path('work/stdshader_dx9-disassembly.txt').read_text().splitlines()
spans=[]
def function(begin,end):
 cursor=begin
 for n,line in enumerate(lines):
  match=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
  if match and begin<=int(match[1],16)<end:
   address=int(match[1],16);data=bytes.fromhex(match[2])
   continuation=re.fullmatch(r'\s+((?:[0-9A-F]{2}\s*)+)',lines[n+1])
   if continuation:data+=bytes.fromhex(continuation[1])
   assert address==cursor,(hex(address),hex(cursor))
   at=image.offset(address);assert image.b[at:at+len(data)]==data
   spans.append({'address':hex(address),'bytes':data.hex()});cursor+=len(data)
 assert cursor==end,(hex(cursor),hex(end))

function(0x10053af0,0x10053b61)
function(0x10053bc0,0x10053bf2)
function(0x10018160,0x10018174)
function(0x10059b74,0x10059c8c)
function(0x10053d70,0x10053d98)
function(0x10053eb0,0x10053edf)
function(0x10059c9e,0x10059d97)
function(0x10053e10,0x10053ea3)
function(0x10056b2e,0x10056b4b)
function(0x100568a5,0x100568cc)
function(0x10053da0,0x10053e05)
function(0x10056662,0x10056b2e)
assert image.string(0x1009cab0)=='vertexlit_and_unlit_generic_ps20b'
assert image.string(0x1008c1dc)=='FLASHLIGHT'
fields=[]
for offset,store,code,label,expression,name in [
 (0,0x10059bf4,'8945e4',0x1008b21c,0x1008b230,'COMPRESSED_VERTS'),
 (4,0x10059b9e,'8945e8',0x10090e44,0x10090e54,'DYNAMIC_LIGHT'),
 (8,0x10059bcd,'8945ec',0x1008b268,0x10092ef0,'SKINNING'),
 (12,0x10059c18,'c745f000000000',0x1008f4fc,0x1008b0cc,'TESSELLATION'),
 (16,0x10059c4f,'8945f4',0x1008c1f8,0x1008d054,'NUM_LIGHTS')]:
 data=bytes.fromhex(code);at=image.offset(store)
 assert image.b[at:at+len(data)]==data
 assert image.string(label)==name
 spans.append({'address':hex(store),'bytes':code})
 fields.append({'offset':offset,'store':hex(store),'nearby_debug_label':name,'debug_expression':image.string(expression),'label_address':hex(label)})
report={'binary':str(path),'sha256':hashlib.sha256(image.b).hexdigest(),
 'static_constructor':'0x10053af0','static_initialization':
 {'offsets_0_through_40':'zero','offset_44':'first signed hardware query at vtable byte0x44 >=92 and material params[0] word at byte8 has bit31 clear','offset_48':'second signed hardware query at vtable byte0x44 <92'},
 'dynamic_constructor':'0x10053bc0','dynamic_initialization':
 {'offsets_0_through_16':'zero','offset_20':'argument interface query at vtable byte8 equals2'},
 'pixel_static_selection':{'packer':'10053e10,24 words, scale8','helper_selection':'10056b39 packer -> shadow slot18 with name vertexlit_and_unlit_generic_ps20b','flashlight':'word7 (byte0x1c), store100568ab in local struct frame-0x7c, adjacent FLASHLIGHT debug label','limits':'Other field semantics/domains and upstream material policy not yet recovered.'},
 'pixel_dynamic_selection':{
 'ps20':'constructor10053d70 initializes offset0=0, offset4=(query slot2==2); inline10059cf4 outputs WRITEWATERFOGTODESTALPHA+2*implicitQueryBit after command11',
 'ps20b':'constructor10053eb0 initializes offsets0/4=0, offset8=(query slot2==2); inline10059d8a outputs FLASHLIGHTSHADOWS+2*(WRITEWATERFOGTODESTALPHA+2*implicitQueryBit) after command11',
 'limits':'Scoped inline arithmetic/constructor evidence. Upstream water-write/shadow flag policy and static pixel selection remain unfinished.'},
 'dynamic_field_labels':fields,'dynamic_non_bump_vs20_derivation':{
 'COMPRESSED_VERTS':'helper stack argument at ebp+0x24 copied unchanged',
 'DYNAMIC_LIGHT':'light-state byte4 !=0 or signed word0 >0; original helper0x10018160',
 'SKINNING':'signed bone-count local >0',
 'TESSELLATION':'zero',
 'NUM_LIGHTS':'hardware byte result from vtable byte0x0c !=0 ?0: light-state word0',
 'implicit_offset_20':'retains constructor result; interface query equals2'},'verified_instruction_spans':spans,
 'limits':'Offline byte-checked recovery. Hardware queries are separate calls, not assumed identical. Last two static and last dynamic semantic names remain unknown. Nearby diagnostic labels support field interpretation only in the inspected non-bump VS20 branch. Full material branch policy, legal domains, constructor execution oracle and GPU behavior remain unverified.'}
pixelFields=[]
for n,line in enumerate(lines):
 match=re.search(r'(1005[0-9A-F]{4}): .*mov +dword ptr \[ebp-([0-9A-F]+)h\],',line)
 if not match or not (0x1005666a<=int(match[1],16)<0x10056b2e and 0x20<=int(match[2],16)<=0x7c):continue
 pushes=[]
 for following in lines[n+1:n+18]:
  push=re.search(r'push +([0-9A-F]{8})h',following)
  if push:
   address=int(push[1],16)
   if address==0x1008b49c:break
   pushes.append(address)
 if pushes:pixelFields.append({'word':(0x7c-int(match[2],16))//4,'store':hex(int(match[1],16)),'label_address':hex(pushes[-1]),'nearby_debug_label':image.string(pushes[-1])})
assert len(pixelFields)==23 and len({field['word'] for field in pixelFields})==23
report['pixel_static_field_labels']=pixelFields
report['pixel_static_constructor']={'address':'0x10053da0','words0_through22':'zero','word23':'material params[0] flags word at byte8 has bit31 clear','limits':'Byte recovery only. Implicit word23 semantic name and full legal domains remain unresolved.'}
Path('outputs/sfm64-generic-shader-combo-initializers-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('initializer_instruction_spans='+str(len(spans))+' dynamic_field_labels='+str(len(fields)))
