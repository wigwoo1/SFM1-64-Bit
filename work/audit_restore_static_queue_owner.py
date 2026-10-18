from pathlib import Path
import json,struct
exec(Path('work/audit_restore_queue_counter_references.py').read_text().split('selected=[]')[0])
def body(start,end):
 selected=[r for r in rows if start<=int(r['address'],16)<end]
 cursor=start
 for row in selected:
  address=int(row['address'],16);raw=bytes.fromhex(row['bytes'])
  assert address==cursor
  assert im.b[im.offset(address):im.offset(address)+len(raw)]==raw
  cursor+=len(raw)
 assert cursor==end
 return selected
startup=body(0x100f94a0,0x100f94b6)
constructor=body(0x10027240,0x100275f1)
getter=body(0x1000df50,0x1000df56)
assert bytes.fromhex(startup[0]['bytes'])==b'\xb9'+struct.pack('<I',0x10180460)
call=bytes.fromhex(startup[1]['bytes']);assert call[0]==0xe8
assert 0x100f94a5+5+struct.unpack_from('<i',call,1)[0]==0x10027240
assert bytes.fromhex(getter[0]['bytes'])==b'\xb8'+struct.pack('<I',0x10180aa8)
assert 0x10180460+0x648==0x10180aa8
queue_refs=[]
for row in rows:
 if not any(f'{value:X}h' in row['instruction'] for value in range(0x10180aa8,0x10180ac8,4)):continue
 address=int(row['address'],16);raw=bytes.fromhex(row['bytes'])
 assert im.b[im.offset(address):im.offset(address)+len(raw)]==raw
 queue_refs.append(row)
pe=im.u32(60);opt=pe+24;sections=[]
for i in range(im.u16(pe+6)):
 o=opt+im.u16(pe+20)+40*i
 name=im.b[o:o+8].split(b'\0')[0].decode();virtual_size,rva,raw_size,raw_offset=struct.unpack_from('<IIII',im.b,o+8)
 if rva<=0x180aa8<rva+max(virtual_size,raw_size):
  sections.append(dict(name=name,rva=hex(rva),virtual_size=virtual_size,raw_size=raw_size,queue_raw_backed=0x180aa8+32<=rva+raw_size))
report=dict(sha256=sha,verified_startup=startup,verified_constructor=constructor,verified_command_getter=getter,verified_static_queue_references=queue_refs,queue_section=sections,owner='0x10180460',queue_offset='0x648',static_queue='0x10180aa8',finding='Viewport static command state and main static shader owner queue subobject are the same production storage. Separate static viewport and owner command queues in composed oracle fixtures are controlled separation, not faithful production aliasing.',limits='Static address equality and complete constructor byte coverage only. Constructor callees, subsequent initialization, consumer, enable mutation and GPU resource lifetime remain unresolved. No consumer absence or disabled queue inference.',next_action='Bind viewport globals to the composed original owner command subobject and share the x64 command services; verify interleaved queue packet order, capacity/refill and full owner snapshot.')
Path('outputs/sfm64-restore-static-queue-owner.json').write_text(json.dumps(report,indent=2)+'\n')
print('static_owner_queue_alias_verified=1 constructor_bytes='+str(0x3b1)+' static_queue_references='+str(len(queue_refs)))
