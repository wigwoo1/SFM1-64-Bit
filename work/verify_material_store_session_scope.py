"""Owned actual-store publication scope; native scalar setter/value comparison.
Native rollback/store/proxy ownership is NOT inferred from these value records.
"""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,random,re,struct,subprocess,tempfile,time
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges
from verify_material_numeric_value import trunc32
from verify_material_session_backup import quiet,same_float

ROOT=Path(__file__).resolve().parent.parent;OUT=ROOT/'outputs/material-store-session-scope-20261027'
LIB=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/materialsystem.dll'
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
NEW=['material_parameter_override_candidate.h','test_material_store_session_scope.cpp','probe_material_session_scope_reference.cpp','build_material_store_session_scope.cmd','verify_material_store_session_scope.py']
def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def need(b,s):
 if not b:raise RuntimeError(s)
def decode(raw,count):
 need(struct.unpack_from('<I',raw)[0]==count,'wire case count');at=4;records=[]
 for _ in range(count):
  stages=[]
  for _ in range(3):
   need(at+28<=len(raw),'wire header');words=list(struct.unpack_from('<7I',raw,at));at+=28;n=words.pop();s=raw[at:at+n];at+=n;need(len(s)==n,'wire text extent');stages.append((words,s))
  records.append(stages)
 need(at==len(raw),'wire tail');return records
def main():
 pre=json.loads((OUT/'preflight.json').read_text());parent=json.loads(Path(pre['parent']).read_text());allowed=set(pre['intended_changes']);protected={p:h for p,h in pre['protected'].items() if p not in allowed}
 need(sha(pre['parent'])==pre['parent_sha256']=='8eeecd9888a59fdba59bc942a840ff767bfd6bdbe7423c7932875d68a4cc6f16','accepted cached composition receipt')
 need(all(sha(p)==h for p,h in protected.items()),'all unrelated preceding inputs before')
 need(sha(OUT/'before/material_object_store_candidate.h')==pre['protected'][str(ROOT/'work/material_object_store_candidate.h')],'old store retained')
 frozen=ROOT/'outputs/material-cached-backup-composition-20261027/frozen-i5ovrjso';manifest=json.loads((frozen/'manifest.json').read_text());need(all(sha(frozen/p)==h for p,h in manifest.items()),'prior frozen archive unchanged')
 need(sha(LIB)=='c78e414891b4d87e3922610a8bf937bca3efbaab95bced488522045998bc87f8','historical reference image identity')
 sources=source_closure([Path(p) for p in parent['source']]+[ROOT/'work'/n for n in NEW]);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 _,ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[('source_string_constructor',0x10017780,0x10017842),('cached_scalar_getter',0x10017c20,0x10017c6a),('actual_SetFloat',0x10019350,0x10019456),('backing_cleanup',0x10017ad0,0x10017b46)])
 (run/'static-reference.json').write_text(json.dumps(ranges,indent=2)+'\n')
 started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_store_session_scope.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180);(run/'build-driver.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'fresh five builds and owned asm')
 exes=[]
 for tag,name,machine in [('reference','scope_reference_x86',0x14c),('owned','scope_owned_x64',0x8664),('store','store_regression_x64',0x8664),('parameters','parameters_regression_x64',0x8664),('cached','cached_regression_x64',0x8664)]:
  exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];need(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'fresh architecture/asInvoker '+tag)
  log=(OUT/(tag+'-build.log')).read_text();(run/(tag+'-build.log')).write_text(log);need(not re.search(r'\b(?:warning|error)\b',log,re.I),'clean build '+tag)
  inspection=subprocess.run([str(DUMP),'/headers','/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(tag+'-inspection.txt')).write_text(inspection);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',inspection,re.M|re.I);need(modules==['KERNEL32.dll'],'owned/reference system-only imports '+tag);exes.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),imports=modules))
 def execute(args,label,timeout=180):
  p=subprocess.run(list(map(str,args)),cwd=run,capture_output=True,text=True,timeout=timeout);(run/(label+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,label+' failed '+p.stderr);return p.stdout
 safety=execute([exes[1]['path']],'scope-safety');m=re.search(r'checks=(\d+) actual_allocation_failures=(\d+)',safety);need(m and int(m[2])>20,'scope safety/allocation sweep')
 store=execute([exes[2]['path'],'--test'],'store-regression');need('real_queue_capacity=4096 stale_target_refused=1' in store,'existing store creation/queue/stale ownership safety')
 parameters=execute([exes[3]['path'],'--test'],'parameter-regression');need('object_parameter_safety_checks=' in parameters and 'actual_allocation_failures=' in parameters,'existing parameter allocation/reentry safety')
 cached=execute([exes[4]['path']],'cached-regression');need('checks=20 plus_prior=49' in cached,'all69 cached/string/store safety checks')
 strings=[b'.25',b'-0',b'1',b'16777217',b'2147483648',b'-2147483649',b'1e40',b'texture/name',b'1e+']
 values=[0,0x80000000,1,0x80000001,0x3f400000,0x3f800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fa12345,0xffa54321,0x7fc12345,0xffc54321,0x4b800001,0xcf000001]
 cases=[(s,v) for s in strings for v in values];rng=random.Random(2026102755)
 for _ in range(256):cases.append((rng.choice(strings),rng.getrandbits(32)))
 query=run/'query.bin';query.write_bytes(struct.pack('<I',len(cases))+b''.join(struct.pack('<I',len(s))+s+struct.pack('<I',v) for s,v in cases))
 native=execute([exes[0]['path'],LIB,query,run/'native.bin'],'native-values');execute([exes[1]['path'],'--wire',query,run/'owned.bin'],'owned-store-values')
 a=(run/'native.bin').read_bytes();b=(run/'owned.bin').read_bytes();n=decode(a,len(cases));o=decode(b,len(cases))
 if a!=b:
  different=[i for i in range(len(cases)) if n[i]!=o[i]];i=different[0];(run/'difference.json').write_text(json.dumps(dict(count=len(different),case_index=i,query=[cases[i][0].decode(),hex(cases[i][1])],native=[(w,s.hex()) for w,s in n[i]],owned=[(w,s.hex()) for w,s in o[i]]),indent=2));raise RuntimeError('native setter vs actual owned store phases differ; inspect difference.json')
 analytical=0;changed_type=0;changed_cached_integer=0
 for (source,bits),stages in zip(cases,n):
  initial,applied,restored=stages;need(initial[0][0]==0x41 and initial[1]==source,'original string retained before scope');analytical+=1
  expected_applied=([0x40,trunc32(bits),bits,bits,bits,bits],b'');need(applied==expected_applied,'independent applied float cache/type');analytical+=1
  backup=quiet(initial[0][2]);next_bits=bits if same_float(bits,backup) else backup
  need(restored==([0x40,trunc32(next_bits),next_bits,next_bits,next_bits,next_bits],b''),'native ordered equality / typed restore projection');analytical+=1
  changed_type+=restored[0][0]!=initial[0][0];changed_cached_integer+=restored[0][1]!=initial[0][1]
 counts=re.search(r'allocations=(\d+) releases=(\d+)',native);need(counts and counts[1]==counts[2]==str(len(cases)),'native string backing released exactly once')
 need(all(sha(p)==h for p,h in sources.items()) and all(sha(p)==h for p,h in protected.items()),'all unrelated/new sources and original inputs unchanged after')
 need(sha(pre['editor'])==pre['editor_sha256'] and all(sha(ROOT/p)==h for p,h in pre['editor_dependencies'].items()),'editor313 dependencies unchanged')
 result=dict(status='VERIFIED owned single-material session scope and scalar native-value comparison; PARTIALLY VERIFIED complete native lifetime/application',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',scope_safety_checks=int(m[1]),actual_allocation_failures=int(m[2]),wire_cases=len(cases),wire_phases=3*len(cases),exact_wire_bytes=len(a),analytical_checks=analytical,native_allocations=int(counts[1]),native_releases=int(counts[2]),successful_restore_changed_type_cases=changed_type,successful_restore_changed_integer_cache_cases=changed_cached_integer,scope_safety=safety.strip(),store_regression=store.strip(),parameter_regression=parameters.strip(),cached_regression=cached.strip(),source=sources,protected=protected,parent=pre['parent'],parent_sha256=pre['parent_sha256'],changed_store=dict(path=str(ROOT/'work/material_object_store_candidate.h'),before_sha256=sha(OUT/'before/material_object_store_candidate.h'),after_sha256=sha(ROOT/'work/material_object_store_candidate.h')),executables=exes,query_sha256=sha(query),output_sha256=sha(run/'native.bin'),editor=pre['editor'],editor_sha256=pre['editor_sha256'],editor_dependencies=pre['editor_dependencies'],editor_rebuilt=False,limits=['Actual owned store publishes applied parameters and retargets existing proxy bindings; observer borrows them only within a live owner-thread call','Failure restores original parameter pointer/forward values; success follows existing native-typed restore setters, not original raw types/text/caches','Original native reference executes string constructor/GetFloat/SetFloat/SetFloat/Clear on ownerless variables; it is NOT an original complete store transaction or native rollback guarantee','Native rollback, multi-material/shared-resource collection atomicity, queued engine ownership, GPU, shader-resource initialization and document history are unverified','Texture type5 and unsupported resource writes explicitly refuse; no SetString substitution or fabricated handles','Proxy evaluation and destructive/reference/queue mutations on the target are refused while scoped; queue draining is deferred until all scopes finish','External source-root edits are detected and cause parameter rollback, but deliberately explicit callback edits are not undone; caller must keep store alive and not retain borrowed state references','Forward snapshots use existing owned calculation; native nonfinite forwarding and complete renderer parity are not established','No original writes/live SFM/computer control/GPU/editor execution/dependency installs/publication'])
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','scope_safety_checks','actual_allocation_failures','wire_cases','exact_wire_bytes','analytical_checks','successful_restore_changed_type_cases','successful_restore_changed_integer_cache_cases']},indent=2),flush=True)
if __name__=='__main__':main()
