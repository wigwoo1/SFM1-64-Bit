"""Prepare original refill body for controlled allocator/publisher execution."""
from pathlib import Path
import hashlib,json,struct
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
report=json.loads(Path('outputs/sfm64-device-queue-refill-recovery.json').read_text())
assert report['binary_sha256']==hashlib.sha256(im.b).hexdigest()
start,end=0x10002da0,0x10002dec
raw=im.b[im.offset(start):im.offset(start)+end-start]
verified=bytearray()
for span in report['verified_instruction_spans']:
 address=int(span['address'],16)
 if start<=address<end:
  assert address==start+len(verified)
  verified.extend(bytes.fromhex(span['bytes']))
assert bytes(verified)==raw
assert raw[0x27]==0xb9 and struct.unpack_from('<I',raw,0x28)[0]==0x1017e8b8
calls=[(0x2c,0x10002d30),(0x35,0x10002ce0)]
for offset,target in calls:
 assert raw[offset]==0xe8 and start+offset+5+struct.unpack_from('<i',raw,offset+1)[0]==target
header='#pragma once\nstatic constexpr unsigned char nativePixelQueueRefillBytes[]={'+','.join(map(str,raw))+'};\n'
header+='static constexpr unsigned nativePixelQueueRefillFileOffset='+str(im.offset(start))+';\n'
Path('work/pixel_queue_refill_native_init.h').write_text(header)
Path('outputs/sfm64-pixel-queue-refill-native-preparation.json').write_text(json.dumps({
 'installed_sha256':report['binary_sha256'],'body_bytes':len(raw),'global_pointer_operand_offset':40,
 'calls':[{'offset':o,'target':hex(t)} for o,t in calls],
 'controlled_bindings':'Publish10002d30 and acquire10002ce0 use thiscall with one stack argument. Replace ring pointer operand and direct calls; original refill remains intact.',
 'limits':'Preparation only; no execution. Controlled callbacks cannot establish real allocator, consumer, concurrency or GPU parity.'},indent=2))
print('pixel_queue_refill_prepared_bytes='+str(len(raw)))
