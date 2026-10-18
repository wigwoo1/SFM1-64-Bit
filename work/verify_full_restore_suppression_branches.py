from pathlib import Path
import json,hashlib
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest();assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
source=json.loads(Path('outputs/sfm64-owner-reset-cleanup-recovery.json').read_text());receivers=json.loads(Path('outputs/sfm64-full-restore-command-receivers.json').read_text());assert source['sha256']==receivers['sha256']==sha
rows=[r for r in source['instructions'] if 0x1001fb90<=int(r['address'],16)<0x100211e7];by={int(r['address'],16):r for r in rows}
gates={s['call'] for s in receivers['sites'] if s['slot']==36};regions=[]
for i,r in enumerate(rows):
 if r['address'] not in gates:continue
 call=int(r['address'],16);test,branch=rows[i+1:i+3];assert test['bytes']=='84c0' and branch['instruction'].startswith('jne')
 a=int(branch['address'],16);b=bytes.fromhex(branch['bytes']);assert b[0]==0x75;target=a+len(b)+int.from_bytes(b[1:],'little',signed=True);assert target in by and target>a
 body=[x for x in rows if a+len(b)<=int(x['address'],16)<target]
 for x in [r,test,branch]+body:
  raw=bytes.fromhex(x['bytes']);offset=im.offset(int(x['address'],16));assert im.b[offset:offset+len(raw)]==raw
 regions.append({'gate_call':r['address'],'test':test,'branch':branch,'nonzero_skips_to':hex(target),'zero_falls_through_to':hex(a+len(b)),'enclosed_calls':[x for x in body if x['instruction'].startswith('call')],'queue_enable_checks':[x for x in body if x['instruction']=='cmp         dword ptr [ebx+00000658h],0']})
assert len(regions)==18 and sum(bool(r['queue_enable_checks']) for r in regions)==16
Path('outputs/sfm64-full-restore-suppression-branches.json').write_text(json.dumps({'sha256':sha,'regions':regions,'meaning':'Owner slot36 true suppresses these operation regions. False permits later cache/queue tests; owner658 independently selects queued versus direct device submission. Last two suppressed regions invoke callback arrays.','limits':'Local branch bytes and enclosed call inventory only, not proof that all operation side effects are gated or whole routine execution. Existing gate fixture verifies predicate separately.'},indent=2)+'\n')
print('full_restore_suppression_regions=18 queue_selection_regions=16 verified')
