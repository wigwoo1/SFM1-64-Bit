from pathlib import Path
import runpy,re,json
cfg=runpy.run_path('work/verify_full_restore_saved_receiver_cfg.py')
by,pred,reachable,entry,sha=(cfg[k] for k in ('by','pred','reachable','entry','sha'))
definitions={}
for a,r in by.items():
 m=re.match(r'(\w+)\s+esi(?:,|$)',r['instruction'])
 if not m or m[1] in ('push','test','cmp','call'):continue
 assert m[1] in ('mov','lea','add','shl','pop'),r
 if r['instruction'] in ('lea         esi,[ebx+000002D0h]','mov         esi,dword ptr [ebp-40h]'):token='owner_subobject_2d0'
 elif r['instruction']=='mov         esi,dword ptr [ebx+00000648h]':token='live_device_owner648'
 else:token='other_definition_'+hex(a)
 definitions[a]=token
incoming={a:set() for a in reachable};outgoing={a:set() for a in reachable};changed=True
while changed:
 changed=False
 for a in sorted(reachable):
  new={'entry_unknown'} if a==entry else set().union(*(outgoing[p] for p in pred[a]))
  after={definitions[a]} if a in definitions else new
  if new!=incoming[a] or after!=outgoing[a]:incoming[a]=new;outgoing[a]=after;changed=True
audit=json.loads(Path('outputs/sfm64-full-restore-indirect-slot-audit.json').read_text());assert audit['sha256']==sha
sites=[]
for s in audit['sites']:
 v=s.get('nearest_vtable_register_load',[])
 if not v or not re.fullmatch(r'mov\s+(eax|ecx|edx),dword ptr \[esi\]',v[0]['instruction']):continue
 a=int(v[0]['address'],16);call=int(s['call'],16)
 assert incoming[a]==incoming[call] and len(incoming[a])==1
 token=next(iter(incoming[a]));assert token in ('owner_subobject_2d0','live_device_owner648'),(s,token)
 sites.append({'call':s['call'],'slot':s['slot'],'vtable_load':v[0],'receiver':token,'all_normal_paths_agree':True})
assert len(sites)==23
report={'sha256':sha,'esi_definitions':{hex(a):v for a,v in definitions.items()},'sites':sites,'assumptions':'Normal CFG and x86 calling convention: callees preserve ESI. Saved stack-local owner+2d0 initialization proven by prior CFG verifier.','limits':'Does not establish method behavior, exception paths or arbitrary ABI-violating callbacks. Other receiver registers and fullfunction execution remain unverified.'}
Path('outputs/sfm64-full-restore-esi-receiver-flow.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'full_restore_esi_receiver_sites={len(sites)} reaching_definitions_verified')
