
from pathlib import Path
import json
exec(Path('work/audit_restore_queue_counter_references.py').read_text().split('selected=[]')[0])
enable=[];addresses=[];constructor=[]
for i,row in enumerate(rows):
 a=int(row['address'],16);text=row['instruction']
 selected=('00000658h' in text and '[esp' not in text) or ('lea' in text and '00000648h' in text) or 0x10027240<=a<0x1002728e
 if not selected:continue
 raw=bytes.fromhex(row['bytes']);assert im.b[im.offset(a):im.offset(a)+len(raw)]==raw
 item={**row,'following':rows[i+1:i+6]}
 if '00000658h' in text and '[esp' not in text:enable.append(item)
 if 'lea' in text and '00000648h' in text:addresses.append(item)
 if 0x10027240<=a<0x1002728e:constructor.append(row)
assert enable and all(r['instruction'].startswith('cmp') for r in enable)
assert any(r['address']=='0x10027245' and r['instruction'].endswith('100296F0') for r in constructor)
assert any(r['address']=='0x1002726f' and '100FD33Ch' in r['instruction'] for r in constructor)
assert any(r['address']=='0x10027275' and '100FCEDCh' in r['instruction'] for r in constructor)
report={'sha256':sha,'direct_owner_enable_reads':enable,'queue_subobject_address_candidates':addresses,'verified_constructor_prefix':constructor,
 'finding':'All displayed non-stack owner displacement +658 accesses are comparisons against zero; no direct +658 store identified. Constructor prefix invokes base100296f0 and embedded initializers, then installs owner/embedded tables.',
 'limits':'Queue address candidates carry local following context, not complete receiver or alias proofs. Shifted pointers can access enable as +10; writes via callbacks, external modules, memory copies or initialization remain unclassified. This does not prove queue execution is dormant or disabled.',
 'next_action':'Resolve address-candidate callees and initialization storage, including the indirect callback array passed queue subobject by full restore.'}
Path('outputs/sfm64-restore-queue-enable-ownership.json').write_text(json.dumps(report,indent=2)+chr(10))
print('verified_owner_enable_comparisons='+str(len(enable))+' queue_address_candidates='+str(len(addresses))+' constructor_prefix_instructions='+str(len(constructor)))
