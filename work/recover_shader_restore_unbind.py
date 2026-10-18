from pathlib import Path
import json,runpy
context=runpy.run_path('work/recover_constant_subobject_methods.py')
image=context['image'];verify=context['verified_span']
regions=[]
for name,start,end in [('manager9',0x10034130,0x100341c5),('manager10',0x100341f0,0x100342c8),('vertex_binding',0x10033d90,0x10033e42),('pixel_binding',0x10033e50,0x10033f02)]:
 rows=verify(start,end);starts={int(r['address'],16) for r in rows}
 for row in rows:
  if row['instruction'].startswith('j'):
   assert int(row['instruction'].split()[1],16) in starts
 regions.append(dict(name=name,start=hex(start),end_exclusive=hex(end),verified_bytes=end-start,instructions=rows,calls=[r for r in rows if r['instruction'].startswith('call')]))
assert image.word(0x100ff9b4+9*4)==0x10034130 and image.word(0x100ff9b4+10*4)==0x100341f0
report=dict(dll_sha256=context['expected'],regions=regions,restore_path='Manager9/10 argumentffffffff follows early branch, calls10033d90/10033e50 with shader=nullptr and secondarg0. Full selection paths also captured, not implemented.',binding_policy='Compare shader pointer with manager cached9c/a0; equal returns. Otherwise lazy initialize external counter and increment, retrieve command state1000df50. Queue-enabled emits opcode6(vertex)/5(pixel) then native32 shader pointer, or direct device92(vertex)/107(pixel). Publish cache after submission regardless HRESULT.',pointer_fields=['manager+9c vertex shader pointer','manager+a0 pixel shader pointer'],x64_constraint='Widen cached shader pointers and queued pointer operands consistently with consumer. ffffffff sentinel is forced invalidation state, not a valid shader pointer. Native two-DWORD queue records cannot preserve arbitrary pointers above4GB.',remaining_dependencies='Lazy counter creation via imported services and exit registration100df012, command-state getter/refill, actual queue consumer and GPU lifecycle.',limits='Static installed-byte verification only; no native binding-helper execution, counter-lifetime proof or full shader-selection parity.')
Path('outputs/sfm64-shader-restore-unbind-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('shader_restore_unbind_regions=4 verified_bytes='+str(sum(r['verified_bytes'] for r in regions)))
