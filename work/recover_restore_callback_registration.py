from pathlib import Path
import re, json, hashlib, struct
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest()
assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
begin,end=0x10026890,0x100268f7
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
assert cursor==end and bytes.fromhex(rows[-1]['bytes'])==bytes([0xc2,8,0])

assert end-begin==103
assert {c['target'] for c in calls}=={'0x1002ef20','0x100d73c0'}
slots={hex(table):[slot for slot in range(count) if im.word(table+slot*4)==begin] for table,count in [(0x100fd33c,598),(0x100fcedc,280),(0x100fcec0,7)]}
# Registration is a direct helper; no matching target in checked interface tables.
full={}
exec(Path('work/audit_restore_queue_counter_references.py').read_text().split('selected=[]')[0],full)
registrations=[]
for i,row in enumerate(full['rows']):
 if row['instruction']!='call        10026890':continue
 context=full['rows'][i-4:i+1]
 for item in context:
  a=int(item['address'],16);b=bytes.fromhex(item['bytes']);assert im.b[im.offset(a):im.offset(a)+len(b)]==b
 registrations.append({'call':row['address'],'context':context})
assert len(registrations)==3
report={'sha256':sha,'begin':hex(begin),'end':hex(end),'verified_bytes':end-begin,'instructions':rows,'direct_calls':calls,'internal_branches':edges,'checked_interface_slot_matches':slots,'verified_registration_calls':registrations,'flow':['Select callback vector at owner+2e8c+20*kind.','Capture old count; grow capacity through1002ef20 when old count+1 exceeds capacity.','Increment live count and update vector element alias at+10.','If callback mutates count while growing, move the remaining live elements through100d73c0.','Store caller supplied callback pointer at captured old-count index.'],'limits':'Exact static registration body and three direct registration contexts; no matching target in checked owner/embedded tables. Callback identities are caller supplied, not fixed installed targets; bounds on kind, allocator effects, callback growth mutation and runtime registration callers need execution/binding. This body does not directly write queue enable.'}
Path('outputs/sfm64-restore-callback-registration-recovery.json').write_text(json.dumps(report,indent=2)+chr(10))
print('callback_registration_bytes=103 direct_calls='+str(len(calls))+' checked_interface_matches='+str(slots))
