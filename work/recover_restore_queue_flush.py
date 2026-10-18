from pathlib import Path
import re, json, hashlib, struct
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest()
assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
begin,end=0x10002e50,0x10002eb1
rows=[]; pending=None
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
 if m:
  pending=None; a=int(m[1],16)
  if begin<=a<end:
   pending={'address':hex(a),'bytes':bytes.fromhex(m[2]).hex(),'instruction':m[3].strip()};rows.append(pending)
 elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):pending['bytes']+=bytes.fromhex(line).hex()
cursor=begin; calls=[]; edges=[]; starts={int(r['address'],16) for r in rows}
for r in rows:
 a=int(r['address'],16); b=bytes.fromhex(r['bytes'])
 assert a==cursor and im.b[im.offset(a):im.offset(a)+len(b)]==b
 cursor+=len(b)
 if b[0]==0xe8:calls.append({'address':hex(a),'target':hex(cursor+struct.unpack('<i',b[1:])[0])})
 if b[0] in range(0x70,0x80) or b[0]==0xeb:
  target=cursor+int.from_bytes(b[1:],'little',signed=True)
 elif b[:1]==b'\x0f' and 0x80<=b[1]<=0x8f:
  target=cursor+int.from_bytes(b[2:],'little',signed=True)
 else:continue
 assert target in starts;edges.append({'address':hex(a),'target':hex(target)})
assert cursor==end and bytes.fromhex(rows[-1]['bytes'])==b'\xc3'

assert end-begin==97
assert calls==[{'address':'0x10002e82','target':'0x10002d30'},{'address':'0x10002e8b','target':'0x10002ce0'}]
raw=im.b[im.offset(begin):im.offset(end)]
fixups={46:0x1017e8b8,82:0x1017f188,87:0x1017f290}
for offset,value in fixups.items():assert struct.unpack_from('<I',raw,offset)[0]==value
assert 0x1017e8b8+0x8d0==0x1017f188 and 0x1017e8b8+0x9d8==0x1017f290
report={'sha256':sha,'begin':hex(begin),'end':hex(end),'verified_bytes':len(raw),'instructions':rows,'direct_calls':calls,'internal_branches':edges,'fixups':{str(k):hex(v) for k,v in fixups.items()},'flow':['Queue subobject +0x10 zero returns immediately.','Enabled queue with nonempty current block terminates cursor, sets block header 2, publishes, acquires header 1, sets cursor block+1 and remaining4095.','Enabled queue with null current block acquires header1 and initializes cursor/capacity.','Enabled queue with empty current block skips publication and allocation but still drains.','Drain loop repeatedly reloads global ring produced (+0x8d0) and completed (+0x9d8), returning only on equality; this loop contains no imported wait call.'],'limits':'Fresh exact byte and control-flow recovery; no execution of this flush body yet. Original ordinary loads do not establish portable C++ synchronization or safe x64 memory ordering. Owned policy delegates completion polling and must be bound to verified worker services.'}
Path('outputs/sfm64-restore-queue-flush-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
Path('work/restore_queue_flush_init.h').write_text('static const unsigned queueFlushOffset='+str(im.offset(begin))+';\nstatic const unsigned char queueFlushBytes[]={'+','.join(str(v) for v in raw)+'};\n')
print('queue_flush_bytes=97 direct_calls=2 internal_branches='+str(len(edges))+' ring_counter_bindings=verified')

