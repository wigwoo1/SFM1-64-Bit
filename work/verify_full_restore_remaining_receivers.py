from pathlib import Path
import runpy,json,re
flow=runpy.run_path('work/verify_full_restore_esi_receiver_flow.py');cfg=flow['cfg']
by,dom,sha=(cfg[k] for k in ('by','dom','sha'))
traces=json.loads(Path('outputs/sfm64-full-restore-interface-source-traces.json').read_text());assert traces['sha256']==sha
done={s['call'] for s in flow['sites']};results=[]
for s in traces['sites']:
 if s['call'] in done:continue
 call=int(s['call'],16);slot=s['slot'];vtable=int(s['vtable_load']['address'],16)
 if s['call']=='0x1001fbbf':
  start=0x1001fbba;reg='ecx';binding='owner_subobject_2d0';assert by[start]['bytes']=='8bce' and flow['incoming'][start]=={binding}
 else:
  row=s['linear_origin']['assignment'];start=int(row['address'],16);reg=s['receiver_register_or_embedded_base'];instruction=row['instruction']
  if 'ds:[1016D130h]' in instruction:binding='global_service1016d130'
  elif '[ebx+00000648h]' in instruction:binding='live_device_owner648'
  elif '[ebp-40h]' in instruction:binding='owner_subobject_2d0';assert 0x1001fbbc in dom[start]
  elif instruction=='mov         ecx,esi':binding='owner_subobject_2d0';assert flow['incoming'][start]=={binding}
  else:raise AssertionError(s)
 assert start in dom[call]
 device=binding=='live_device_owner648'
 interval=[(a,r) for a,r in by.items() if start<a<call]
 if device:
  pushes=[a for a,r in interval if re.fullmatch(r'push\s+'+reg,r['instruction'])];assert len(pushes)==1;consume=pushes[0];assert consume in dom[call]
 else:consume=call
 for a,r in interval:
  assert not r['instruction'].startswith('call')
  if a<consume:
   assert not re.match(r'(mov|lea|pop|xor|add|sub|and|or|shl|shr|inc|dec)\s+'+reg+r'(?:,|$)',r['instruction']),r
 results.append({'call':s['call'],'slot':slot,'receiver':binding,'source_instruction':by[start],'receiver_consumed_at':hex(consume),'source_dominates_call':True})
assert len(results)==12
Path('outputs/sfm64-full-restore-remaining-receivers.json').write_text(json.dumps({'sha256':sha,'sites':results,'limits':'Normal intraprocedural CFG, standard ABI and previously verified saved-local initialization. Device receiver is consumed by stack push before possible target-register reuse. Fullowner execution and actual method behavior remain unverified.'},indent=2)+'\n')
print('full_restore_remaining_receivers=12 verified')
