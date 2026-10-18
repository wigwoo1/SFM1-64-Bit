from pathlib import Path
import struct,json
path=Path('build/session-core/sfm64-material-alpha-candidate.spv')
data=path.read_bytes();words=struct.unpack('<'+'I'*(len(data)//4),data)
assert words[0]==0x07230203
offsets=[];push=[];i=5
while i<len(words):
 count=words[i]>>16;opcode=words[i]&65535
 assert count and i+count<=len(words)
 operands=words[i+1:i+count]
 if opcode==72 and len(operands)==4 and operands[2]==35:offsets.append(dict(struct_id=operands[0],member=operands[1],offset=operands[3]))
 if opcode==59 and len(operands)>=3 and operands[2]==9:push.append(operands[1])
 i+=count
assert len(push)==1 and len(offsets)==2 and [v['offset'] for v in offsets]==[96,100]
result=dict(compiled=True,push_constant_variables=len(push),member_offsets=offsets,scope='SPIR-V layout only; GPU execution and native blend/depth parity unverified')
Path('outputs/sfm64-material-alpha-shader-layout.json').write_text(json.dumps(result,indent=2)+'\n')
print('material alpha shader compiled; SPIR-V push constant alpha/cutoff offsets 96/100 verified')
