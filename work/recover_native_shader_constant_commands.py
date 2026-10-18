from pathlib import Path
import hashlib,json
from pe_image import Image
path=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll');p=Image(path);spans=[]
def verify(address,code):
 data=bytes.fromhex(code);at=p.offset(address);assert p.b[at:at+len(data)]==data,hex(address);spans.append({'address':hex(address),'bytes':data.hex()})
assert p.word(0x10018b90+3*4)==0x10016b24
assert p.word(0x10018b90+4*4)==0x10016e85
verify(0x10016b24,'8b 47 04 8b 4f 08')
verify(0x10016b3d,'8d 57 0c 8d 7c df 0c')
verify(0x10016b39,'8b d9 03 db')
verify(0x10016c29,'8b 8d 74 ff ff ff 8b 95 78 ff ff ff 8b 9e 78 03 00 00 8b 03')
verify(0x10016e85,'8b 5f 04 8b 47 08 8b 8e 10 17 00 00')
verify(0x10016ea0,'c1 e0 04 8d 57 0c')
verify(0x10016eba,'8d 7c 07 0c')
verify(0x10016fa2,'8b 8d 74 ff ff ff 8b 9e 78 03 00 00 8b 13')
verify(0x10016fb8,'8b 8a 78 01 00 00 50 53 ff d1')
report={'binary':str(path),'sha256':hashlib.sha256(p.b).hexdigest(),'pixel_constant_opcode':3,'vertex_constant_opcode':4,'layout':'opcode,start-register,register-count,4*register-count uint32 float-bit payload','stride_bytes':'12+16*register-count','vertex_direct_device_vtable_offset':'0x178','opcode7':'different specialized path, not generic constant payload','verified_spans':spans,'limits':'Static operand/stride evidence only. Native constant-cache comparison, skipped prefixes, queued upload semantics, specialized commands, execution oracle and GPU behavior remain unverified.'}
Path('outputs/sfm64-native-shader-constant-command-recovery.json').write_text(json.dumps(report,indent=2)+'\n');print('constant_command_spans='+str(len(spans)))
