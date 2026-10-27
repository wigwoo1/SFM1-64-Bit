"""Actual cached material getters -> pinned wire -> actual IFM typed capture.

Separate processes, same historical-image identities, no full engine lifetime.
The owned path constructs typed storage and ignores poisoned getter-answer fields.
"""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,re,struct,subprocess,tempfile,time
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges
from verify_material_get_string import corpus
from verify_material_session_backup import row,encode,expected,quiet,color

ROOT=Path(__file__).resolve().parent.parent;OUT=ROOT/'outputs/material-cached-backup-composition-20261027'
LIB=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/materialsystem.dll';IFM=LIB.parent/'tools/ifm.dll'
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
NEW=['probe_material_cached_backup.cpp','test_material_cached_backup.cpp','build_material_cached_backup.cmd','verify_material_cached_backup.py']
def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def need(b,s):
 if not b:raise RuntimeError(s)
def text(s):return struct.pack('<I',len(s))+s
def source_cases():
 cases=[c for c in corpus() if c[0] in [0,1,2,4,6,7]]
 # Existing constructor semantics are prerequisites; replay representative stored
 # strings with distinct integer/scalar caches, rather than replacing them by text.
 for s in [b'.25',b'-0',b'16777217',b'2147483648',b'-2147483649',b'1e40',b'-1e40',b'1e-50',b'12.5suffix',b'1e+',b'1d3',b'nan',b'inf',b'0x1p2',b'3\0ignored']:
  for meta in [0,1]:cases.append((1,4,0,0,meta,[0]*16,s,b''))
 return cases
def encode_sources(cases):return struct.pack('<I',len(cases))+b''.join(struct.pack('<21I',*c[:5],*c[5])+text(c[6])+text(c[7]) for c in cases)
def decode_getters(raw,specs):
 need(len(raw)==4+144*len(specs) and struct.unpack_from('<I',raw)[0]==len(specs),'getter record framing');records=[]
 for i,spec in enumerate(specs):
  words=list(struct.unpack_from('<36I',raw,4+144*i));cache=words[:6];integer=words[6];scalar=words[7];vectors=[words[8+j*4:12+j*4] for j in range(3)];matrix=words[20:36]
  need(integer==cache[1] and scalar==quiet(cache[2]),'independent cached integer/scalar projection')
  for j,v in enumerate(vectors):need(v==[quiet(cache[2+k]) if k<j+2 else 0 for k in range(4)],'requested vector count, quiet copy and untouched padding')
  identity=[0x3f800000 if j%5==0 else 0 for j in range(16)]
  need(matrix==(spec[5] if spec[0]==7 else identity),'raw matrix/identity getter projection')
  records.append(dict(cache=cache,integer=integer,scalar=scalar,vectors=vectors,matrix=matrix))
 return records
def template_cases(records):
 cases=[];kinds=[2,3,4,8,9,10,11,14]
 def make(index,kind,slot,profile=0,meta=0):
  observed=records[index];before=[0x13572468+i for i in range(16)];live=[0]*16
  if kind in [2,4]:live[0]=observed['integer']
  elif kind==3:live[0]=observed['scalar']
  elif kind in [8,9,10,11]:live[:4]=observed['vectors'][{8:1,9:0,10:1,11:2}[kind]]
  elif kind==14:live=observed['matrix'].copy()
  if profile:
   if kind==2:before[0]=live[0]
   elif kind==3:before[0]=live[0]
   elif kind==4:before[0]=(before[0]&0xffffff00)|int(live[0]!=0)
   elif kind==8:before[0]=sum(color(live[j])<<(8*j) for j in range(3))
   elif kind in [9,10,11]:before[:{9:2,10:3,11:4}[kind]]=live[:{9:2,10:3,11:4}[kind]]
   elif kind==14:before=live.copy()
  result=row(kind=kind,before=before,live=live,name='$p_'+str(slot),metadata=meta);result['source_index']=index;return result
 for index in range(len(records)):
  for profile in [0,1]:cases.append((3,0,[make(index,k,j,profile) for j,k in enumerate(kinds)]))
 for index in range(min(80,len(records))):
  for phase in [-1,0,1,2,3,4,5]:
   for meta in [0,0x20,0x400,0x80,0x4080,0x2080,0xffff0000]:cases.append((3,phase,[make(index,k,j,index%2,meta) for j,k in enumerate(kinds)]))
 # Ordered cases hit the actual maximum256 retained parameters, without shared
 # raw variable pointers or relying on lookup order to recover the source index.
 for start in range(0,len(records),32):
  cases.append((3,0,[make(index,k,j*8+offset) for j,index in enumerate(range(start,min(start+32,len(records)))) for offset,k in enumerate(kinds)]))
 for flags in range(4):cases.append((flags,0,[make(0,k,j) for j,k in enumerate(kinds)]))
 # Equality differs by target: signed zero compares equal for floats/vectors,
 # but matrix equality is raw-byte equality. NaN matrices can be true no-ops.
 for index,record in enumerate(records[:256]):
  if record['scalar']&0x7fffffff==0:
   rs=[make(index,k,j,1) for j,k in enumerate(kinds)]
   for r in rs:
    n={3:1,9:2,10:3,11:4,14:16}.get(r['flags']&31,0)
    for j in range(n):
     if r['before'][j]&0x7fffffff==0:r['before'][j]^=0x80000000
   cases.append((3,0,rs))
 for kind in [0,1,6,7,12,13,15,31]:cases.append((3,0,[make(0,kind,0)]))
 return cases
def poisoned(cases):
 result=[]
 for flags,phase,rows in cases:
  changed=[]
  for r in rows:
   other=dict(r);other['live']=[0xdeadbeef]*15+[r['source_index']];other['live_text']='POISON: derive cached getters from typed source';changed.append(other)
  result.append((flags,phase,changed))
 return result
def main():
 pre=json.loads((OUT/'preflight.json').read_text());parent=json.loads(Path(pre['parent']).read_text());allowed=set(pre['intended_changes']);protected={p:h for p,h in pre['protected'].items() if p not in allowed}
 need(sha(pre['parent'])==pre['parent_sha256']=='bcef59ce50dab6b2be1be1e4e8c5a92341f2fd604096f9c7eb21dce9f67eab42','accepted predecessor receipt')
 need(all(sha(p)==h for p,h in protected.items()),'all unrelated preceding source/input identities')
 need(sha(OUT/'before/material_parameter_backup_candidate.h')==pre['protected'][str(ROOT/'work/material_parameter_backup_candidate.h')],'old adapter retained')
 need(sha(LIB)=='c78e414891b4d87e3922610a8bf937bca3efbaab95bced488522045998bc87f8' and sha(IFM)=='8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4','historical image identities')
 sources=source_closure([Path(p) for p in parent['source']]+[ROOT/'work'/n for n in NEW]);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 mi,mr=check_ranges(LIB,'materialsystem-disassembly.txt',[('GetInt',0x10017bd0,0x10017c1a),('GetFloat_x87',0x10017c20,0x10017c6a),('GetVec_requested_x87',0x10017cc0,0x10017d77),('GetMatrix_raw_or_identity',0x10018d60,0x10018e5c)])
 need(all(mi.word(0x10098f34+slot*4)==target for slot,target in [(26,0x10017bd0),(27,0x10017c20),(28,0x10017cc0),(20,0x10018d60)]),'actual getter vtable routes')
 _,ir=check_ranges(IFM,'ifm-disassembly.txt',[('whole_capture',0x1023c0a0,0x1023c286),('mutation_gate',0x103623b0,0x10362421),('matrix_copy_quiet',0x10496870,0x104968da),('matrix_compare_raw',0x104968e0,0x10496950)])
 (run/'static-reference.json').write_text(json.dumps(dict(material=mr,ifm=ir),indent=2)+'\n')
 started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_cached_backup.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180);(run/'build-driver.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'fresh five builds and existing owned asm')
 exes=[]
 for tag,name,machine in [('getter','cached_reference_x86',0x14c),('capture','capture_reference_x86',0x14c),('owned','cached_owned_x64',0x8664),('string','string_regression_owned_x64',0x8664),('safety','cached_backup_safety_x64',0x8664)]:
  exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];need(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'fresh architecture/manifest '+tag)
  log=(OUT/(tag+'-build.log')).read_text();(run/(tag+'-build.log')).write_text(log);need(not re.search(r'\b(?:warning|error)\b',log,re.I),'clean compile '+tag)
  inspect=subprocess.run([str(DUMP),'/headers','/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(tag+'-inspection.txt')).write_text(inspect);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',inspect,re.M|re.I);need(modules==['KERNEL32.dll'],'system-only imports '+tag);exes.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),imports=modules))
 def execute(args,label):
  p=subprocess.run(list(map(str,args)),cwd=run,capture_output=True,text=True,timeout=180);(run/(label+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,label+' failed '+p.stderr);return p.stdout
 safety=execute([exes[4]['path']],'safety');need('plus_prior=49' in safety,'prior49 safety suite and extra getter checks')
 specs=source_cases();source_query=run/'sources.bin';source_query.write_bytes(encode_sources(specs));logs=[]
 for tag,e,image in [('reference',exes[0],LIB),('owned',exes[2],run/'absent.dll')]:logs.append(execute([e['path'],'--getters',image,source_query,run/(tag+'-getters.bin')],tag+'-getters'))
 a=(run/'reference-getters.bin').read_bytes();b=(run/'owned-getters.bin').read_bytes()
 if a!=b:
  at=next((i for i,(x,y) in enumerate(zip(a,b)) if x!=y),min(len(a),len(b)));raise RuntimeError('cached getter mismatch at '+str(at))
 records=decode_getters(a,specs);cases=template_cases(records);native_query=run/'capture-query.bin';native_query.write_bytes(encode(cases));owned_query=run/'owned-query.bin';owned_query.write_bytes(encode(poisoned(cases)));predicted,counts=expected(cases);(run/'expected.bin').write_bytes(predicted)
 native_log=execute([exes[1]['path'],IFM,native_query,run/'capture.bin'],'capture')
 execute([exes[2]['path'],'--capture',run/'absent.dll',source_query,owned_query,run/'owned.bin'],'owned-capture')
 for tag in ['capture','owned']:
  actual=(run/(tag+'.bin')).read_bytes()
  if actual!=predicted:
   at=next((i for i,(x,y) in enumerate(zip(actual,predicted)) if x!=y),min(len(actual),len(predicted)));raise RuntimeError(tag+' typed capture mismatch at '+str(at))
 need('phase_calls='+str(counts['phase_calls'])+' undo_disabled_calls='+str(counts['mutations'])+' notifications_suppressed='+str(counts['mutations']) in native_log,'exact native mutation service counts')
 # Replay every prior typed-to-string capture with a FRESH owned compile and the
 # unchanged native capture fixture. Never revise a historical passing receipt.
 prior=Path(pre['parent']).parent;need(sha(prior/'sources.bin')==parent['boundary']['source_query_sha256'] and sha(prior/'owned-query.bin')==parent['boundary']['owned_poisoned_query_sha256'] and sha(prior/'capture-query.bin')==parent['boundary']['native_capture_query_sha256'],'pinned complete previous string transport')
 execute([exes[3]['path'],run/'absent.dll',prior/'sources.bin',prior/'owned-query.bin',run/'prior-owned.bin'],'prior-owned')
 execute([exes[1]['path'],IFM,prior/'capture-query.bin',run/'prior-native.bin'],'prior-native')
 need(sha(run/'prior-owned.bin')==sha(run/'prior-native.bin')==parent['boundary']['native_capture_output_sha256'],'all prior14382 string captures unchanged')
 allocations=re.search(r'allocations=(\d+) releases=(\d+)',logs[0]);need(allocations and allocations[1]==allocations[2],'all native backing allocations released')
 need(all(sha(p)==h for p,h in sources.items()) and all(sha(p)==h for p,h in protected.items()),'all source/unchanged inputs after')
 need(sha(pre['editor'])==pre['editor_sha256'] and all(sha(ROOT/p)==h for p,h in pre['editor_dependencies'].items()),'editor313 dependencies unchanged')
 result=dict(status='VERIFIED cached-getter and nonstring typed-capture composition; PARTIALLY VERIFIED full lifecycle/application',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',getter_cases=len(specs),getter_output_bytes=len(a),capture_cases=len(cases),capture_attributes=sum(len(rs) for _,_,rs in cases),exact_output_bytes=len(predicted),**counts,safety=safety.strip(),native_allocations=int(allocations[1]),native_releases=int(allocations[2]),per_target_attributes={str(k):sum((r['flags']&31)==k for _,_,rs in cases for r in rs) for k in range(32)},prior_string_cases=parent['capture_cases'],prior_string_bytes=parent['exact_output_bytes'],source=sources,protected=protected,parent=pre['parent'],parent_sha256=pre['parent_sha256'],changed={str(ROOT/'work/material_parameter_backup_candidate.h'):dict(before=sha(OUT/'before/material_parameter_backup_candidate.h'),after=sha(ROOT/'work/material_parameter_backup_candidate.h'))},executables=exes,boundary=dict(source_query_sha256=sha(source_query),actual_getter_sha256=sha(run/'reference-getters.bin'),native_capture_query_sha256=sha(native_query),owned_poisoned_query_sha256=sha(owned_query),native_capture_output_sha256=sha(run/'capture.bin'),owned_output_sha256=sha(run/'owned.bin')),editor=pre['editor'],editor_sha256=pre['editor_sha256'],editor_dependencies=pre['editor_dependencies'],editor_rebuilt=False,limits=['Actual original cached getters execute after existing original constructors/matrix setter with null owner and null queue; no queued redirection or resource constructors','Resource-free/string/matrix values admitted; resources3/8 and unsupported storage deliberately refused for numeric reads, not replaced by zero','Matrix getter returns raw bits; IFM matrix equality is bytewise, mutation copy quiets signaling values; scalar/vector equality is numeric','Native vector writes exactly requested count even if stored count differs; unused returned owned array slots are zero padding, not native returned bytes','Historical MaterialSystem and IFM execute in separate processes with pinned byte transport; not one native engine instance','Native FindVar/getter transport, phase, disabled undo, suppressed notifications remain services; no real document-history/notification ownership integration','No editor/GPU/visual equivalence, production per-draw transaction, original writes, live SFM/computer use, dependency changes or publication'])
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','getter_cases','capture_cases','capture_attributes','exact_output_bytes','native_allocations','safety']},indent=2),flush=True)
if __name__=='__main__':main()
