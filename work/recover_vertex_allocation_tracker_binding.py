from pathlib import Path
import json,struct
exec(Path('work/audit_restore_queue_counter_references.py').read_text().split('selected=[]')[0])
global_address=0x10190934
pair_offset=0x179ff0
name_address=im.u32(pair_offset)
assert im.u32(pair_offset+4)==global_address and im.string(name_address)=='VBAllocTracker001'
references=[]
for i,row in enumerate(rows):
 if '10190934h' not in row['instruction']:continue
 context=rows[max(0,i-3):i+12]
 for entry in context:
  address=int(entry['address'],16);raw=bytes.fromhex(entry['bytes'])
  assert im.b[im.offset(address):im.offset(address)+len(raw)]==raw
 references.append(dict(reference=row,context=context))
needle=struct.pack('<I',global_address);offset=0;occurrences=[]
while True:
 offset=im.b.find(needle,offset)
 if offset<0:break
 occurrences.append(offset);offset+=1
assert pair_offset+4 in occurrences
report=dict(sha256=sha,interface='VBAllocTracker001',global_address=hex(global_address),pair_file_offset=pair_offset,name_address=hex(name_address),verified_references=references,raw_pointer_occurrences=occurrences,
finding='Constructor global is paired with VBAllocTracker001 in an interface-name/output-cell table. Selected callers use slot0 registration, slot1 resource removal, slot2 string or null context changes. Semantic labels are inferred from arguments and call sites, not proven provider API.',
limits='Global lacks file backing. Exact displayed references are reads; no direct writer or provider established here. Interface string is present in many DLL shared binding tables, which does not identify provider. Runtime factory wiring and aliased stores remain unresolved.',
next_action='Trace interface-table connection routine and find VBAllocTracker001 factory registration/provider before implementing resource tracker lifetime.')
Path('outputs/sfm64-vertex-allocation-tracker-binding.json').write_text(json.dumps(report,indent=2)+'\n')
print('verified_tracker_references='+str(len(references))+' raw_occurrences='+str(len(occurrences))+' interface=VBAllocTracker001')
