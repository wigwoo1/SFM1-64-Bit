from pathlib import Path
import json,hashlib
from pe_image import Image
apiPath=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
shaderPath=apiPath.with_name('stdshader_dx9.dll')
api=Image(apiPath);shader=Image(shaderPath);spans=[]
def verify(image,address,hexbytes):
 data=bytes.fromhex(hexbytes);at=image.offset(address);assert image.b[at:at+len(data)]==data,hex(address)
 spans.append({'binary':'shaderapi' if image is api else 'stdshader','address':hex(address),'bytes':data.hex()})
assert api.string(0x1016de3c)=='.?AVCShaderAPIDx8@@'
col=api.word(0x100fcedc-4)
assert col==0x1016aa20 and api.word(col+4)==720 and api.word(col+12)==0x1016de34
assert api.word(0x100fcedc+52*4)==0x10016ac0
verify(shader,0x1005a7dd,'c7 00 00 00 00 00')
verify(shader,0x1005a7e3,'8b 01 8b 80 d0 00 00 00')
verify(shader,0x1005a7ef,'8d 95 98 fb ff ff 52 ff d0')
verify(shader,0x10059c6f,'c7 00 0c 00 00 00')
verify(shader,0x10059c85,'e8 76 9f ff ff')
verify(shader,0x10059c8a,'89 06')
verify(api,0x10016ad6,'8b 7d 08 8b f1')
verify(api,0x10016ae4,'8b 07 83 f8 10 77 f9 ff 24 85 90 8b 01 10')
table={command:hex(api.word(0x10018b90+command*4)) for command in range(17)}
assert table[0]=='0x10016af2' and table[11]=='0x10017039' and table[12]=='0x1001704d'
verify(api,0x10017039,'8b 4f 04 8b 15 0c e2 16 10 89 4a 08 83 c7 08 e9 97 fa ff ff')
verify(api,0x1001704d,'8b 47 04 8b 0d 0c e2 16 10 89 41 04 83 c7 08 e9 83 fa ff ff')
verify(api,0x10016af2,'8b 85 40 ff ff ff 8d 4d cc 3b c1 0f 84 77 20 00 00')
verify(api,0x10016b03,'8b 38 83 c0 04 eb d4')
verify(api,0x10016b0a,'8b 7f 04 eb d5')
report={'binaries':{str(apiPath):hashlib.sha256(api.b).hexdigest(),str(shaderPath):hashlib.sha256(shader.b).hexdigest()},'class':'CShaderAPIDx8','interface_vtable':'0x100fcedc','RTTI_subobject_offset':720,'executor_slot':52,'executor':'0x10016ac0','producer_execution_call':'0x1005a7f6','dispatch_table':'0x10018b90','dispatch':table,'shader_manager_global':'0x1016e20c','dynamic_vertex_command':{'opcode':12,'operand':'following uint32','write':'manager+4','advance_bytes':8,'producer':'0x10059c6f + packer0x10053c00 result'},'dynamic_pixel_command':{'opcode':11,'operand':'following uint32','write':'manager+8','advance_bytes':8},'terminator':{'opcode':0,'behavior':'return if nested-buffer stack empty; otherwise pop previous stream'},'jump_command':{'opcode':1,'operand':'following native pointer'},'verified_spans':spans,'limits':'Static dispatch/producer/store evidence only. No complete command interpreter or native execution oracle. Pixel producer, nested call semantics, manager consumption during GPU binding and other commands remain unverified. Owned x64 interpreter must not reuse 32-bit pointer operands as x64 pointers.'}
Path('outputs/sfm64-native-dynamic-shader-command-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('dynamic_shader_command_spans='+str(len(spans))+' command12=manager+4 command11=manager+8')
