from pathlib import Path
import json,re,struct,hashlib
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest()
assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
specs=[('refill',0x10002da0,76,5120,'work/restore_refill_body_init.h'),('publish',0x10002d30,105,5632,'work/restore_publish_init.h'),('acquire',0x10002ce0,77,6144,'work/restore_acquire_init.h')]
image=bytearray([0xcc]*8192);bodies=[]
for name,address,size,destination,header in specs:
 raw=im.b[im.offset(address):im.offset(address)+size]
 m=re.search(r'Bytes\[\]=\{([^}]+)\}',Path(header).read_text())
 assert m and raw==bytes(int(v) for v in m[1].split(',')) and len(raw)==size
 image[destination:destination+size]=raw
 bodies.append(dict(name=name,address=hex(address),size=size,destination=destination,sha256=hashlib.sha256(raw).hexdigest()))
links=[(5120,0x10002da0,44,0x10002d30,5632),(5120,0x10002da0,53,0x10002ce0,6144),(6144,0x10002ce0,41,0x10002da0,5120)]
for destination,address,call,target,new_target in links:
 assert image[destination+call]==0xe8
 assert address+call+5+struct.unpack_from('<i',image,destination+call+1)[0]==target
 struct.pack_into('<i',image,destination+call+1,new_target-(destination+call+5))
 assert destination+call+5+struct.unpack_from('<i',image,destination+call+1)[0]==new_target
bindings=[('ring',5120+40,0x1017e8b8),('wait_cell',5632+30,0x100fa0d4),('wait_cell',6144+6,0x100fa0d4)]+[('pool',6144+v,0x1017e040) for v in (21,55,68)]
for name,offset,original in bindings:
 assert struct.unpack_from('<I',image,offset)[0]==original
 struct.pack_into('<I',image,offset,0)
report=dict(sha256=sha,bodies=bodies,internal_links=[dict(call_offset=d+c,target_offset=t) for d,a,c,o,t in links],required_runtime_bindings=[dict(name=n,operand_offset=o,original=hex(v)) for n,o,v in bindings],image_size=len(image),finding='Freshly verified linked original refill/publisher/allocator bundle, with allocator-to-refill recursion retained. External ring/pool/wait pointer operands explicitly cleared until binding.',limits='Preparation and relative-link verification only. This artifact is not runnable without bindings; no original execution, boundary behavior, consumer, concurrency or GPU claim.',next_action='Install into existing shared-owner restoration oracle and compare capacity crossings against composed pointer-wide refill/publisher/allocator services.')
Path('outputs/restore-shared-queue-refill-template.bin').write_bytes(image)
Path('outputs/sfm64-restore-shared-queue-refill-preparation.json').write_text(json.dumps(report,indent=2)+'\n')
header='// Verified original-code offsets within the 8192-byte private oracle image.\n'
header+='static const unsigned sharedRefillOffset=5120,sharedPublishOffset=5632,sharedAcquireOffset=6144;\n'
header+='static const unsigned sharedRefillLinks[][2]={'+','.join('{'+str(d+c+1)+','+str(t)+'}' for d,a,c,o,t in links)+'};\n'
header+='static const unsigned sharedRefillBindings[][2]={'+','.join('{'+str(o)+','+str({'ring':0,'pool':1,'wait_cell':2}[n])+'}' for n,o,v in bindings)+'};\n'
Path('work/restore_shared_queue_refill_init.h').write_text(header)
print('verified_original_refill_chain_bytes='+str(sum(b['size'] for b in bodies))+' internal_links=3 external_operands=6 prepared_not_executed=1')
