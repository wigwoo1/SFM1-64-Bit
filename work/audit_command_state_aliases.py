from pathlib import Path
import re,json,struct
exec(Path('work/audit_restore_queue_counter_references.py').read_text().split('selected=[]')[0])
def verify(row):
 a=int(row['address'],16);raw=bytes.fromhex(row['bytes'])
 assert im.b[im.offset(a):im.offset(a)+len(raw)]==raw
 return row
assert im.b[im.offset(0x1000df50):im.offset(0x1000df50)+6]==bytes.fromhex('b8a80a1810c3')
registers='eax ebx ecx edx esi edi ebp esp'.split()
traces=[];accesses=[];handoffs=[];stored_pointers=[]
for i,row in enumerate(rows):
 if row['instruction']!='call        1000DF50':continue
 verify(row);raw=bytes.fromhex(row['bytes']);assert raw[0]==0xe8 and int(row['address'],16)+5+struct.unpack_from('<i',raw,1)[0]==0x1000df50
 aliases={'eax':0};trace=[];stop='window limit';pending_pushes=[];next_address=int(row['address'],16)+len(raw)
 for nextrow in rows[i+1:i+49]:
  if int(nextrow['address'],16)!=next_address:stop='disassembly coverage gap';break
  verify(nextrow);next_address+=len(bytes.fromhex(nextrow['bytes']));trace.append(nextrow);text=nextrow['instruction'];parts=text.split(None,1);op=parts[0];args=parts[1] if len(parts)>1 else ''
  # Bounded straight-line analysis: stop rather than guess across calls/branches.
  if op.startswith('j') or op in ('ret','int','loop','leave'):
   stop='control flow boundary';break
  for reg,offset in list(aliases.items()):
   for mem in re.finditer(r'\[('+reg+r')(?:\+([0-9A-F]+)h)?\]',args):
    displacement=int(mem[2],16) if mem[2] else 0
    first=args.split(',')[0]
    role='read' if op in ('cmp','test','push') or '[' not in first else 'write_or_read_modify_write'
    if op=='lea':role='address'
    accesses.append(dict(getter_call=row['address'],instruction=nextrow,register=reg,queue_offset=hex(offset+displacement),role=role))
  operands=args.split(',')
  if op=='mov' and len(operands)==2 and '[' in operands[0] and operands[1] in aliases:stored_pointers.append(dict(getter_call=row['address'],instruction=nextrow,queue_offset=hex(aliases[operands[1]])))
  if op=='push' and args in aliases:pending_pushes.append(dict(register=args,queue_offset=hex(aliases[args]),instruction=nextrow))
  if op=='call':
   handoffs.append(dict(getter_call=row['address'],call=nextrow,ecx_queue_offset=hex(aliases['ecx']) if 'ecx' in aliases else None,pushed_queue_pointers=pending_pushes))
   stop='call boundary';break
  m=re.fullmatch(r'('+ '|'.join(registers)+r'),('+ '|'.join(registers)+r')',args)
  if op=='mov' and m:
   if m[2] in aliases:aliases[m[1]]=aliases[m[2]]
   else:aliases.pop(m[1],None)
   continue
  m=re.fullmatch(r'('+ '|'.join(registers)+r'),\[('+ '|'.join(registers)+r')(?:\+([0-9A-F]+)h)?\]',args)
  if op=='lea' and m:
   if m[2] in aliases:aliases[m[1]]=aliases[m[2]]+(int(m[3],16) if m[3] else 0)
   else:aliases.pop(m[1],None)
   continue
  m=re.fullmatch(r'('+ '|'.join(registers)+r'),([0-9A-F]+)h?',args)
  if op in ('add','sub') and m and m[1] in aliases:
   aliases[m[1]]+=(1 if op=='add' else -1)*int(m[2],16);continue
  # Unknown writes to registers invalidate aliases conservatively.
  destination=args.split(',')[0]
  if destination in registers and op not in ('cmp','test','push','nop'):aliases.pop(destination,None)
  if not aliases:stop='all aliases invalidated';break
 traces.append(dict(getter_call=row,verified_trace=trace,stop=stop))
flag_accesses=[r for r in accesses if r['queue_offset']=='0x10']
report=dict(sha256=sha,getter='0x1000df50',traces=traces,relative_accesses=accesses,queue_enable_accesses=flag_accesses,handoffs=handoffs,stored_queue_pointer_candidates=stored_pointers,limits='Bounded straight-line register-copy/constant-offset analysis. Stops at first branch/call; stack aliases, indexed accesses, nonconstant arithmetic and callee semantics unresolved. Role is syntactic; no whole-program absence or runtime activation claim.',next_action='Inspect verified handoff callees and branch-separated aliases; inspect interface-owned initialization independently.')
Path('outputs/sfm64-command-state-alias-audit.json').write_text(json.dumps(report,indent=2)+'\n')
print('verified_getter_traces='+str(len(traces))+' relative_accesses='+str(len(accesses))+' queue_enable_accesses='+str(len(flag_accesses))+' call_handoffs='+str(len(handoffs)))
print('stored_queue_pointer_candidates='+str(len(stored_pointers)))
print('enable_write_candidates='+str(sum(r['role']=='write_or_read_modify_write' for r in flag_accesses)))
print('handoff_targets='+str(sorted(set(h['call']['instruction'] for h in handoffs if h['ecx_queue_offset'] is not None or h['pushed_queue_pointers']))))
