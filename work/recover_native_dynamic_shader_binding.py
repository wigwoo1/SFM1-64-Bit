from pathlib import Path
import hashlib,json
from pe_image import Image
path=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll');p=Image(path);spans=[]
def verify(address,code):
 data=bytes.fromhex(code);at=p.offset(address);assert p.b[at:at+len(data)]==data,hex(address)
 spans.append({'address':hex(address),'bytes':data.hex()})
assert p.word(0x100ff9b4+9*4)==0x10034130
assert p.word(0x100ff9b4+10*4)==0x100341f0
verify(0x1003413d,'83 fb ff 75 12')
verify(0x10034146,'e8 45 fc ff ff')
verify(0x10034154,'8b 71 04 85 f6 79 02 33 f6')
verify(0x1003415d,'f6 43 18 02 75 59')
verify(0x10034163,'8b 43 10 8b 3c b0')
verify(0x100341b1,'8b 4d fc 6a 00 57 e8 d4 fb ff ff')
verify(0x10034216,'8b 47 08 89 45 08 3b 43 08 7e 20')
verify(0x10034241,'f6 43 18 02 74 17')
verify(0x1003425e,'8b 4b 10 8b 34 81')
verify(0x100342b5,'6a 00 56 8b cf e8 91 fb ff ff')
verify(0x10033d9a,'39 bb 9c 00 00 00 0f 84 96 00 00 00')
verify(0x10033e04,'c7 00 06 00 00 00')
verify(0x10033e27,'8b 36 8b 0e 8b 91 70 01 00 00 57 56 ff d2')
verify(0x10033e35,'89 bb 9c 00 00 00')
verify(0x10033e5a,'39 bb a0 00 00 00 0f 84 96 00 00 00')
verify(0x10033ec4,'c7 00 05 00 00 00')
verify(0x10033ee7,'8b 36 8b 0e 8b 91 ac 01 00 00 57 56 ff d2')
verify(0x10033ef5,'89 bb a0 00 00 00')
report={'binary':str(path),'sha256':hashlib.sha256(p.b).hexdigest(),'vertex':{'manager_slot':9,'select':'0x10034130','dynamic_index':'manager+4; negative values normalize to0','resources':'record+0x10 pointer array indexed with dynamic*4','disabled_flag':'record+0x18 bit1 skips selection','null_handle':'0xffffffff calls binding helper with null','bind':'0x10033d90','last_resource':'manager+0x9c','queued_opcode':6,'direct_device_vtable_offset':'0x170 (slot92 in x86)'},'pixel':{'manager_slot':10,'select':'0x100341f0','dynamic_index':'manager+8; compares signed index against record+8 before selecting','resources':'record+0x10 pointer array indexed with dynamic*4','disabled_flag':'record+0x18 bit1 emits diagnostic and returns','bind':'0x10033e50','last_resource':'manager+0xa0','queued_opcode':5,'direct_device_vtable_offset':'0x1ac (slot107 in x86)'},'verified_spans':spans,'limits':'Offline instruction evidence only. Pointer-array population, unusual native bounds semantics, lazy creation dependencies, queued command consumer, HRESULT behavior and GPU execution remain unverified. Owned x64 code must retain safe bounds and owned resource references rather than copy native pointer ABI.'}
Path('outputs/sfm64-native-dynamic-shader-binding-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('dynamic_shader_binding_spans='+str(len(spans)))
