from pathlib import Path
import hashlib,json,re
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest()
assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
expected={0x1002df70:{49,69},0x1002e010:{57},0x1002e070:{57},0x1002e0e0:{57},0x1002e170:{69},0x1002eac0:{1,262},0x1002eb00:{263},0x1002eb30:{272},0x1002eb60:{264},0x1002e1f0:{57,271,41,17,25,264},0x1002e510:{261},0x1002e580:{57},0x1002e700:{57},0x1002e8e0:{57}}
reports=[]
for path in ['outputs/sfm64-transition-handler-table-recovery.json','outputs/sfm64-transition-wrapper-helper-recovery.json']:
 source=json.loads(Path(path).read_text());assert source['sha256']==sha
 for h in source['handlers']:
  begin=int(h['begin'],16);sites=[];rows=h['instructions']
  for row in rows:
   a=int(row['address'],16);b=bytes.fromhex(row['bytes']);assert im.b[im.offset(a):im.offset(a)+len(b)]==b
  for index,row in enumerate(rows):
   m=re.fullmatch(r'call\s+(eax|ecx|edx)',row['instruction'])
   if not m:continue
   register=m[1];load=None
   for previous in reversed(rows[max(0,index-40):index]):
    assignment=re.match(r'mov\s+'+register+r',(.+)',previous['instruction'])
    if not assignment:continue
    operand=re.fullmatch(r'dword ptr \[(eax|ecx|edx)\+([0-9A-F]+)(h?)\]',assignment[1])
    assert operand,('Unclassified call target',row,previous)
    displacement=int(operand[2],16 if operand[3] else 10);assert displacement%4==0
    load={'address':previous['address'],'instruction':previous['instruction'],'byte_offset':displacement,'slot':displacement//4}
    break
   assert load,('Missing target load',row)
   sites.append({'call':row['address'],'target_load':load,'context':rows[max(0,index-10):index+1]})
  slots={site['target_load']['slot'] for site in sites}
  assert slots==expected.get(begin,set()),(hex(begin),slots,expected.get(begin,set()))
  reports.append({'begin':hex(begin),'slots':sorted(slots),'sites':sites})
out={'sha256':sha,'pointer_size':4,'bodies':reports,'depth_correction':'1002e510 target offset0x414 divided by4 is slot261, superseding older slot265 prose.','limits':'Slot arithmetic and original bytes checked. Object identity comes from surrounding context; this audit does not execute backend methods or establish full renderer parity.'}
Path('outputs/sfm64-transition-indirect-slot-audit.json').write_text(json.dumps(out,indent=2)+'\n')
print(f'transition_indirect_bodies={len(reports)} call_sites={sum(len(h["sites"]) for h in reports)} slot_checks=passed')


