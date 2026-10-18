from pathlib import Path
import struct,json
exec(Path('work/recover_command_buffer_creation.py').read_text().split('vertex=region')[0])
lookup={int(r['address'],16):r for r in rows}
results=[]
for start in [0x100e01aa,0x100e1154]:
 todo=[start];seen=set();calls=[]
 while todo:
  a=todo.pop()
  if a in seen:continue
  assert a in lookup;seen.add(a);r=lookup[a];b=bytes.fromhex(r['bytes']);assert b==im.b[im.offset(a):im.offset(a)+len(b)]
  op=r['instruction'].split()[0];fall=a+len(b)
  if op=='ret':continue
  if op.startswith('j'):
   if b[0] in [0xe9,0xeb]:target=fall+struct.unpack('<i' if b[0]==0xe9 else '<b',b[1:])[0]
   elif b[0]==0x0f:target=fall+struct.unpack('<i',b[2:])[0]
   else:target=fall+struct.unpack('<b',b[1:])[0]
   todo.append(target)
   if op!='jmp':todo.append(fall)
  else:
   if op=='call':calls.append(r)
   todo.append(fall)
 end=max(a+len(bytes.fromhex(lookup[a]['bytes'])) for a in seen)
 print(hex(start),hex(end),'instructions',len(seen),'calls',calls)
 results.append(dict(start=hex(start),end=hex(end),reachable_instructions=[lookup[a] for a in sorted(seen)],calls=calls))
Path('outputs/sfm64-optimized-power-reachable.json').write_text(json.dumps(dict(sha256=sha,routines=results),indent=2)+'\n')
