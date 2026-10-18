from pathlib import Path
import hashlib,json,struct
from pe_image import Image
image=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
report=json.loads(Path('outputs/sfm64-shader-restore-unbind-recovery.json').read_text())
assert hashlib.sha256(image.b).hexdigest()==report['dll_sha256']
header='#pragma once\n';prepared=[]
for index,(start,end,target) in enumerate([(0x10034130,0x10034154,0x10033d90),(0x100341f0,0x10034216,0x10033e50)]):
 rows=[r for r in report['regions'][index]['instructions'] if start<=int(r['address'],16)<end]
 covered={};calls=[];guards=[]
 raw=image.b[image.offset(start):image.offset(start)+end-start]
 for row in rows:
  address=int(row['address'],16);covered.update({address+i:v for i,v in enumerate(bytes.fromhex(row['bytes']))})
  if row['instruction'].startswith('call'):
   actual=int(row['instruction'].split()[1],16);offset=address-start
   assert actual==target and raw[offset]==0xe8
   assert address+5+struct.unpack_from('<i',raw,offset+1)[0]==target
   calls.append(dict(offset=offset,target=target))
  if row['instruction'].startswith('jne'):
   assert int(row['instruction'].split()[1],16)==end
   offset=address-start;assert raw[offset]==0x75 and address+2+raw[offset+1]==end
   guards.append(dict(offset=offset,trap_offset=end-start))
 assert raw==bytes(covered[a] for a in range(start,end))
 assert len(calls)==len(guards)==1 and rows[-1]['instruction']=='ret         4'
 assert bytes.fromhex(rows[-1]['bytes'])==bytes.fromhex('c20400')
 header+=f'static const unsigned char managerUnbindPrefix{index}[]={{'+','.join(map(str,raw))+'};\n'
 prepared.append(dict(file_offset=image.offset(start),bytes=len(raw),binding_call=calls[0],invalid_handle_guard=guards[0],limits='Only ffffffff sentinel path copied; non-sentinel branch must land on explicit trap, not missing selection code.'))
header+='struct ManagerUnbindPrefixBody{const unsigned char* bytes;unsigned size,fileOffset,callOffset,target;};\n'
header+='static const ManagerUnbindPrefixBody managerUnbindPrefixBodies[]={'+','.join('{managerUnbindPrefix'+str(i)+','+str(p['bytes'])+','+str(p['file_offset'])+','+str(p['binding_call']['offset'])+','+str(p['binding_call']['target'])+'}' for i,p in enumerate(prepared))+'};\n'
Path('work/manager_unbind_prefix_native_init.h').write_text(header)
Path('outputs/sfm64-manager-unbind-prefix-native-preparation.json').write_text(json.dumps(dict(dll_sha256=report['dll_sha256'],prefixes=prepared,native_executed=False,scope='Restore sentinel path only; complete shader selection remains required elsewhere.'),indent=2)+'\n')
print('manager_unbind_prefix_bytes='+str(sum(p['bytes'] for p in prepared))+' verified')
