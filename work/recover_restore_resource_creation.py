from pathlib import Path
import re, json, hashlib, struct
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest()
assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
begin,end=0x10019b80,0x10019db3
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
assert {c['target'] for c in calls}=={'0x10002e50','0x10036f30'}
prior=json.loads(Path('outputs/sfm64-owner-reset-cleanup-recovery.json').read_text())
assert prior['sha256']==sha
binding=prior['import_bindings']['0x100fa244']
report={'sha256':sha,'begin':hex(begin),'end':hex(end),'verified_bytes':end-begin,'instructions':rows,'direct_calls':calls,'internal_branches':edges,'diagnostic':{'string':im.string(0x100fc91c),'import':binding},
 'observed_flow':[
 'Optional diagnostic gated by global10183c64+30. Resource array pointer is owner+668; first slot nonzero skips first creation path.',
 'Positive signed global10183ab4+30 selects padded path: queue flush10002e50 then device slot18 writes resource array+8; owner slot3 obtains dimensions, each extended by twice live convar value; device-global10183b5c slot2 feeds format helper10036f30; queue flush then live device slot28 writes resource array+0 with dimensions, mapped format, captured owner40/44 and trailing zeros.',
 'Nonpositive padding path: queue flush then device slot38 arguments0/resource array+0; reload resource array and clear slot+8 after callback.',
 'Global1016d130 slot34 result2 and live resource array+4 null gate second creation. Owner slot3 obtains dimensions; queue flush/device slot28 format0x71 uses captured owner40/44 and writes array+4. Nonzero result or success with output still null retries slot28 with owner40/44 arguments zero, using prior dimension locals.',
 'Owner+680 null gates queue flush then live device slot40 output directly at owner+680.'],
 'slot_offsets':{'device':'18/28/38/40 correspond vtable byte offsets48/70/98/a0; method identities inferred from argument shapes, not established here','owner':'3 at byte offset0c','service1016d130':'34 at byte offset88'},
 'limits':'Exact static body and internal control flow checked against installed bytes. No original execution or x64 policy parity. Resource/device interface identities, HRESULT effects, queue publication and consumer synchronization and callback mutation ownership require further binding.'}
Path('outputs/restore-resource-creation-original.bin').write_bytes(im.b[im.offset(begin):im.offset(end)])
Path('outputs/sfm64-restore-resource-creation-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'restore_resource_creation_bytes={end-begin} direct_calls={len(calls)} internal_branches={len(edges)} verified')

