from pathlib import Path
import struct,json
b=Path('build/session-core/sfm64-eye-projection-candidate.spv').read_bytes();words=struct.unpack('<'+'I'*(len(b)//4),b);assert words[:2]==(0x07230203,0x00010000)
members=[];locations=[];dots=0;i=5
while i<len(words):
 length,op=words[i]>>16,words[i]&65535;assert length and i+length<=len(words);args=words[i+1:i+length]
 if op==72 and len(args)==4 and args[2]==35:members.append(list(args))
 if op==71 and len(args)==3 and args[1]==30:locations.append(list(args))
 if op==148:dots+=1
 i+=length
assert sorted(m[3] for m in members)==[0,16,32,48,64,80]
assert dots==6
assert any(a[2]==1 for a in locations)
Path('outputs/sfm64-eye-projection-shader-validation.json').write_text(json.dumps({'spirv_bytes':len(b),'push_member_offsets':[0,16,32,48,64,80],'dot_products':dots,'iris_output_location_present':True,'gpu_execution':'not yet tested','native_projection_vector_generation':'unfinished'},indent=2)+'\n');print('Eye projection shader compiled: six explicit row dot products and 96-byte push layout verified')
