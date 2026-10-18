from pathlib import Path
from pe_image import Image
import struct,hashlib,json
path=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\stdshader_dx9.dll');p=Image(path)
def verify(address,hexbytes):
 b=bytes.fromhex(hexbytes);assert p.b[p.offset(address):p.offset(address)+len(b)]==b,hex(address);return {'address':hex(address),'bytes':b.hex()}
assert p.string(0x100bc0d4)=='.?AVCShader@VertexLitGeneric@@'
assert p.word(0x1009c39c+14*4)==0x100537a0
assert p.word(p.word(0x1009c39c-4)+12)==0x100bc0cc
call=verify(0x10053862,'e8 b9 6f 00 00');assert 0x10053862+5+struct.unpack('<i',bytes.fromhex('b96f0000'))[0]==0x1005a820
assert struct.unpack_from('<f',p.b,p.offset(0x1008b218))[0]==0
records=[call,verify(0x1005a861,'e8 8a d8 fd ff'),verify(0x100382ba,'8b 50 30'),verify(0x100382c0,'ff d2'),verify(0x100382c2,'8b 46 58 83 f8 ff 74 22'),verify(0x100382ca,'8b 04 83 f3 0f 10 40 0c'),verify(0x100382d2,'0f 2f 05 18 b2 08 10 76 11'),verify(0x100382db,'8b 17 8b 42 34 51 f3 0f 11 04 24 6a 06 8b cf ff d0')]
assert p.string(0x1008f618)=='$ALPHATESTREFERENCE'
assert p.string(0x1008b1bc)=='0.0'
records.extend([verify(0x10085220,'6a 00 68 b2 ae 08 10 68 bc b1 08 10 6a 07 68 18 f6 08 10 b9 0c a8 0c 10 e8 13 e8 fc ff'),verify(0x10053a53,'8b 45 08'),verify(0x10053a5f,'89 06'),verify(0x10053a73,'8b 15 2c ad 0c 10 83 c2 0d 89 56 14'),verify(0x10052fde,'8b 0d 20 a8 0c 10 89 48 58'),verify(0x10053845,'e8 c6 f6 ff ff')])
assert p.word(0x1009c39c+6*4)==0x100652c0
records.extend([verify(0x100652e4,'89 1d 40 b9 0d 10'),verify(0x100652fb,'3b de 74 07 8b cf e8 aa f7 ff ff'),verify(0x1006530c,'8b 17 8b 52 38'),verify(0x1006531e,'ff d2'),verify(0x10064ab0,'8b 0d 40 b9 0d 10 8b 01 8b 10 56 ff d2')])
report={'binary':str(path),'sha256':hashlib.sha256(p.b).hexdigest(),'class':'CShader@VertexLitGeneric','class_vtable':hex(0x1009c39c),'class_method':hex(0x100537a0),'helper_call':hex(0x1005a820),'reference_site':hex(0x100382c2),'helper_call_chain':['0x100537a0','0x1005a820','0x100380f0'],'parameter_index_field':'helper info+0x58','material_float_field':'parameter+0x0c','selection':'skip index -1; skip reference <=0 or unordered; otherwise call shadow slot13 with comparison6 and reference','named_parameter':'$ALPHATESTREFERENCE','parameter_descriptor':hex(0x100ca80c),'parameter_index_location':hex(0x100ca820),'descriptor_default_text':'0.0','binding':'registration constructor stores name at descriptor+0 and assigned index at descriptor+0x14; helper initializer copies index to info+0x58','draw_wrapper':hex(0x100652c0),'conditional_reset':'non-null shadow argument: wrapper calls 0x10064ab0, which calls shadow slot0, before shader virtual slot14','verified_code':records,'limits':'Static class/helper link and reference branch only. Conditional wrapper reset and static helper chain verified. Complete branch reachability, post-reset state changes and native GPU behavior remain unverified.'}
Path('outputs/sfm64-stdshader-alpha-selection-recovery.json').write_text(json.dumps(report,indent=2)+'\n');print('stdshader_alpha_selection_verified_spans='+str(len(records)))
