"""Verify refill/pool publication code without initializing native globals."""
import hashlib,json,re
from pathlib import Path
from pe_image import Image
image=Image(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll'))
lines=Path('work/shaderapidx9-disassembly.txt').read_text().splitlines();spans=[]
for begin,end in [(0x10002ce0,0x10002d2d),(0x10002d30,0x10002d99),(0x10002da0,0x10002dec),(0x10002e50,0x10002eb1),(0x100f91c0,0x100f91cd),(0x100f9190,0x100f91bc),(0x100d6250,0x100d62e9)]:
 cursor=begin
 for n,line in enumerate(lines):
  m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
  if not m or not begin<=int(m[1],16)<end:continue
  address=int(m[1],16);data=bytes.fromhex(m[2])
  extra=re.fullmatch(r'\s+((?:[0-9A-F]{2}\s*)+)',lines[n+1])
  if extra:data+=bytes.fromhex(extra[1])
  assert address==cursor;at=image.offset(address);assert image.b[at:at+len(data)]==data
  spans.append({'address':hex(address),'bytes':data.hex()});cursor+=len(data)
 assert cursor==end,(hex(cursor),hex(end))
assert image.string(0x100fa430)=='mem_dumpvballocs'
assert image.string(0x100fa444)=='Dump VB memory allocation stats.'
assert 0x1017e8b8+0x8d0==0x1017f188 and 0x1017e8b8+0x9d8==0x1017f290
report={
 'binary_sha256':hashlib.sha256(image.b).hexdigest(),
 'pool_table':'500 pointers at1017e040, runtime storage without file backing',
 'acquire':'10002ce0 scans for pointed first word0; sets acquired first word to supplied argument1. If all500 occupied calls refill then imported function at100fa0d4 with argument0 and retries.',
 'publish':'10002d30 waits while signed producer-consumer>=500 using same imported zero-argument-value call. Writes pointer at ring index, increments producer, advances index modulo500.',
 'refill':'10002da0: non-null block with cursor!=block+4 gets terminator0 at cursor and state2 at first word, then is published. Null block acquires new; empty existing block returns without refill. New cursor=block+4, remaining word count4095.',
 'flush_wait':'10002e50 returns immediately if queue object word+0x10==0. Otherwise publishes nonempty current block and acquires replacement (or keeps existing empty block), then repeatedly compares globals1017f188/1017f290 until equal. This loop contains no imported yield call.',
 'counter_aliases':'Ring object1017e8b8+0x8d0 is producer1017f188; +0x9d8 is completion1017f290. Static initializer100f91c0 sets both0. Completion consumer increment/reset path remains unresolved.',
 'excluded_initializer':'100f9190 registers console command mem_dumpvballocs with description Dump VB memory allocation stats. Constructor100d6250 stores fourth argument0x4000 in object+0x14 command flags. It is not the queue pool allocation initializer. Earlier report association corrected.',
 'boundary_limit':'Owned batch packer reserves terminator within4096 words; native count4095 excludes header but not explicitly terminator. Actual queue allocation size remains unknown. Exact full-capacity behavior/allocation extent must be verified before claiming byte-for-byte rollover equivalence.',
 'verified_instruction_spans':spans,
 'limits':'Offline code evidence only. Original constructors/import waits are not executed; concurrency ordering, runtime pool allocation, wraparound and GPU consumer behavior remain unresolved.'}
Path('outputs/sfm64-device-queue-refill-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('device_queue_refill_instruction_spans='+str(len(spans)))
