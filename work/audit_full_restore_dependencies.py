from pathlib import Path
from collections import Counter
import json,hashlib
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest();assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
audit=json.loads(Path('outputs/sfm64-owner-reset-cleanup-recovery.json').read_text())
begin,end=0x1001fb90,0x100211e7
rows=[r for r in audit['instructions'] if begin<=int(r['address'],16)<end]
coverage={}
for r in rows:
 a=int(r['address'],16);b=bytes.fromhex(r['bytes']);assert im.b[im.offset(a):im.offset(a)+len(b)]==b
 for i,value in enumerate(b):assert a+i not in coverage;coverage[a+i]=value
assert set(coverage)==set(range(begin,end))
direct=[];indirect=[]
for i,r in enumerate(rows):
 b=bytes.fromhex(r['bytes']);a=int(r['address'],16)
 if b[0]==0xe8:
  assert len(b)==5;target=a+5+int.from_bytes(b[1:],'little',signed=True)
  direct.append({'address':r['address'],'target':hex(target),'context':rows[max(0,i-6):i]})
 elif r['instruction'].startswith('call'):
  assert b[0]==0xff
  indirect.append({'address':r['address'],'instruction':r['instruction'],'context':rows[max(0,i-8):i]})
counts=Counter(r['target'] for r in direct)
bindings={
 '0x1000e550':{'implementation':'work/studio_default_render_state_owner_candidate.h','comparison':'work/test_default_state_composition_native.cpp','remaining':'Fullowner binding and queue refill/actualGPU; combinedinitializer/setter fixture now exists.'},
 '0x10015bd0':{'comparison':'work/test_constant_storage_restore_native.cpp','remaining':'Fullowner integration and nativeallocation/refill/device bindings.'},
 '0x1000e330':{'implementation':'work/studio_shader_render_state_submit_candidate.h','remaining':'Fullowner callsite/cache mapping and combinedexecution.'},
 '0x100d5490':{'implementation':'work/studio_restore_identity_matrix_candidate.h','recovery':'outputs/sfm64-restore-identity-matrix-recovery.json','comparison':'work/test_restore_fill_identity_native.cpp','remaining':'Original identity/fill composition verified256patterns; fullowner field binding and x64 application integration pending.'},
 '0x100df5ad':{'recovery':'outputs/sfm64-full-restore-resource-binding.json','classification':'Compiler stack-cookie check; equal returns, mismatch tails to100e73dc','remaining':'Failure handler not executed; use compiler-generated x64 stack protection.'},
 '0x10019b80':{'implementation':'work/studio_restore_resource_creation_candidate.h','comparison':'work/test_restore_resource_creation_candidate.cpp','recovery':'outputs/sfm64-restore-resource-creation-recovery.json','remaining':'Original/x64 basic12288 padded6144 retry3072 controlled records matched; same fullowner receiver byte-verified. Real device/queueconsumer/fullowner integration pending.'},
 '0x1002f410':{'implementation':'work/studio_restore_blend_candidate.h','comparison':'work/test_restore_transition_composed_compare.cpp','remaining':'24body parent/dispatcher/handlers/commit compared216cases; fullowner subobject mapping/allopcodeparent/actualGPU remain.'},
 '0x100d73a0':{'classification':'Ninebyte tailjump wrapper to100df3a0 bytefill utility','remaining':'Wrapper bytes/target verified; wholefill/helper fastpath and fullowner callsite extents pending.'},
 '0x100df3a0':{'classification':'Bytefill with external zero-fill fastpath100e7322','comparison':'work/test_restore_fill_identity_native.cpp','remaining':'Original scalar/fastpath82240cases verified; fullowner destination/count bindings pending.'},
 '0x10002da0':{'remaining':'Actualqueue refill/consumer/pool ownership; currentoriginal fixtures use sufficient capacity and trap refill.'},
 '0x100df030':{'remaining':'Copy helper original comparison exists; bind fullowner sizes and livebuffers.'}}
for binding in bindings.values():
 for key in ('implementation','comparison'):
  if key in binding:assert Path(binding[key]).is_file()
unclassified=[target for target in counts if target not in bindings]
report={'sha256':sha,'full_span_bytes':end-begin,'direct_target_counts':dict(counts),'direct_calls':direct,'indirect_calls':indirect,
 'existing_binding_artifacts':bindings,'unclassified_direct_targets':unclassified,
 'verified_tail':'10021087..100211d4 is independently original/x64 compared, but does not prove this complete function.',
 'next_action':'Resource helper same-owner binding and compiler cookie classified. Bind real device/queue consumer services and remaining owner virtual dispatch; fullowner combined execution pending. Parent216case and fill82240/identity256 fixtures do not prove fullowner execution.',
 'limits':'Exact byte/context inventory only. A controlled call trace is not equivalent to original nested helper execution or x64 application integration.'}
Path('outputs/sfm64-full-restore-dependency-audit.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({'full_span_bytes':end-begin,'direct_calls':len(direct),'indirect_calls':len(indirect),'targets':dict(counts)}))



