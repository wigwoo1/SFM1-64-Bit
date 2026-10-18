from pathlib import Path
import json,struct
exec(Path('work/audit_restore_queue_counter_references.py').read_text().split('selected=[]')[0])
def region(start,end):
 selected=[r for r in rows if start<=int(r['address'],16)<end];cursor=start;calls=[];branches=[]
 for row in selected:
  a=int(row['address'],16);raw=bytes.fromhex(row['bytes']);assert a==cursor and im.b[im.offset(a):im.offset(a)+len(raw)]==raw;cursor+=len(raw)
  op=row['instruction'].split()[0]
  if raw[0]==0xe8: calls.append(dict(address=row['address'],target=hex(a+5+struct.unpack_from('<i',raw,1)[0])))
  if op.startswith('j'):branches.append(row)
 assert cursor==end
 return dict(start=hex(start),end=hex(end),bytes=end-start,instructions=selected,direct_calls=calls,branches=branches)
vertex=region(0x10005570,0x10005765);index=region(0x10008a40,0x10008b81)
by_address={int(row['address'],16):row for row in rows}
def proof(address,expected):
 row=by_address[address];assert bytes.fromhex(row['bytes'])==bytes.fromhex(expected);return row
proofs=[proof(0x100055cd,'8b7d08'),proof(0x10005699,'8bcf'),proof(0x100056ac,'8b07'),proof(0x100056c1,'8b4168'),proof(0x10005703,'8b3f'),proof(0x10008aab,'8b4d08'),proof(0x10008abd,'8b5d08'),proof(0x10008ac5,'8b03'),proof(0x10008acf,'8b416c'),proof(0x10008b1a,'8b1b')]
for r in (vertex,index):assert sum(c['target']=='0x10002e50' for c in r['direct_calls'])==3
report=dict(sha256=sha,regions=[vertex,index],queue_argument_proofs=proofs,queue_argument='First stack argument [ebp+8], distinct from ECX buffer object receiver',vertex_policy='Store count and stride on receiver, multiply DWORD byte size. Derive usage 8 or208 from dynamic bit, optional bit10 from argument. Flush queue argument, reload device from command[0], invoke COM slot26 with byte size/usage/FVF/pool/output receiver/shared null. Only8876017c or8007000e triggers optional slot5 cleanup after a second flush, third flush and one retry. Final counters and registration service still run; returned value is buffer receiver.',index_policy='Round supplied index count up to even via (count+1)&fffffffe; byte size doubled modulo32. Derive usage8/208 plus optional10 and pool from dynamic bit. Flush then reload device and invoke slot27 with format65h/output receiver/shared null. Same two retry HRESULT values and optional slot5 cleanup/third flush. Nonzero initial HRESULT logs before retry decisions. Non-dynamic allocation counter lazily initialized/updated. Return receiver.',finding='These getter handoffs perform buffer creation with queue flush/device reload. Selected receiver+10 stores are buffer metadata, not writes to command-state queue enable. Neither body directly stores the queue argument for later retention.',limits='Full installed-byte recovery and explicit stack/register/call-site provenance. API slot names inferred from D3D9 conventions; no original execution, imported counter/logging binding, GPU creation or external mutation equivalence. Callee side effects remain unproved; no consumer absence or queue activation conclusion.',next_action='Prepare original buffer-creation oracles with controlled flush, exact COM slots26/27/5, counter/log services, then compare x64 pointer-wide resource/output ownership and retry behavior; continue interface initialization tracing independently.')
Path('outputs/sfm64-command-buffer-creation-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('verified_buffer_creation_bytes='+str(vertex['bytes']+index['bytes'])+' queue_flush_calls=6 decisive_proofs='+str(len(proofs)))
