from pathlib import Path
import json,hashlib,struct,runpy
from pe_image import Image
runpy.run_path('work/recover_restore_resource_creation.py')
runpy.run_path('work/prepare_restore_format_lookup.py')
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest()
recovery=json.loads(Path('outputs/sfm64-restore-resource-creation-recovery.json').read_text())
assert recovery['sha256']==sha
spans=[(0x10019b80,0x10019db3),(0x10036f30,0x10036f3f)]
blob=bytearray(); bodies=[]
for start,end in spans:
 while len(blob)%16:blob.append(0xcc)
 raw=im.b[im.offset(start):im.offset(end)]
 bodies.append({'address':hex(start),'file_offset':im.offset(start),'size':len(raw),'blob_offset':len(blob)})
 blob.extend(raw)
def placement(address):
 for body in bodies:
  a=int(body['address'],16)
  if a<=address<a+body['size']:return body['blob_offset']+address-a
 raise ValueError(hex(address))
calls=[]
for call in recovery['direct_calls']:
 a=int(call['address'],16);target=int(call['target'],16)
 calls.append({'operand':placement(a)+1,'next':placement(a)+5,'target':call['target'],'binding':'queue_flush' if target==0x10002e50 else 'internal_format_lookup'})
assert len(calls)==7
allowed={0x10183c64:'diagnostic_convar',0x100fc91c:'diagnostic_string',0x100fa244:'diagnostic_import_cell',0x10183ab4:'padding_convar',0x10183b5c:'format_device_cell',0x1016d130:'mode_service_cell',0x1016e5d4:'format_table'}
opt=im.u32(60)+24;rva,size=im.u32(opt+136),im.u32(opt+140)
pos=im.offset(im.base+rva);limit=pos+size;fixups=[]
while pos<limit:
 page,length=im.u32(pos),im.u32(pos+4);assert length>=8 and pos+length<=limit
 for at in range(pos+8,pos+length,2):
  entry=im.u16(at);address=im.base+page+(entry&4095)
  if entry>>12!=3 or not any(a<=address<b for a,b in spans):continue
  value=im.word(address);assert value in allowed
  operand=placement(address);assert struct.unpack_from('<I',blob,operand)[0]==value
  fixups.append({'operand':operand,'original':hex(value),'binding':allowed[value]})
 pos+=length
assert pos==limit and {f['binding'] for f in fixups}==set(allowed.values())
for c in calls:
 if c['binding']=='internal_format_lookup':c['blob_target']=placement(int(c['target'],16))
Path('outputs/restore-resource-creation-composed-original.bin').write_bytes(blob)
report={'sha256':sha,'blob_sha256':hashlib.sha256(blob).hexdigest(),'original_bytes':sum(b['size'] for b in bodies),'blob_bytes':len(blob),'bodies':bodies,'calls':calls,'fixups':fixups,'indirect_bindings':{'owner_slot3':'dimension outputs, thiscall','device_slots':'18/28/38/40 COM-style stack arguments','format_device_slot2':'format enum index, thiscall','mode_service_slot34':'mode integer, thiscall'},'limits':'Unpatched preparation only. Runtime loader must verify installed bytes, patch calls and global operands, protect RX and bind all indirect interfaces. Queue flush substitute does not establish consumer synchronization; installed format table window does not establish valid index bounds.'}
Path('outputs/sfm64-restore-resource-creation-native-preparation.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'resource_graph_original_bytes={report["original_bytes"]} calls={len(calls)} fixups={len(fixups)} verified')
