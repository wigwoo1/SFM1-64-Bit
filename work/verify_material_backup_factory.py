"""Whole backup builder + actual constructor/type5 copying; owned policy joins.

The owned copy service is a controlled string-only adapter, not a full cloner.
"""
import hashlib,json,random,re,struct,subprocess,tempfile,time
from pathlib import Path
from datetime import datetime,timezone
from verify_ifm_draw_packet import check_ranges
from verify_builtin_error_material_seed import source_closure
ROOT=Path(__file__).resolve().parent.parent;OUT=ROOT/'outputs/material-backup-factory-20261027'
IFM=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/tools/ifm.dll'
PARENT=ROOT/'outputs/material-element-binding-20261027/verification-8dfax87m/results.json'
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
NEW=['probe_material_backup_factory.cpp','build_material_backup_factory.cmd','verify_material_backup_factory.py']
def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def need(b,s):
 if not b:raise RuntimeError(s)
def source(available=1,mode=0,name=1,file=123,extra=1,text=2):return [available,mode,name,file,extra,text]
def corpus():
 cases=[]
 def add(label,records=None,handles=None,repeats=1,type=15,warm=0,present=1,lookup=0):cases.append(dict(label=label,records=[source()] if records is None else records,handles=[1] if handles is None else handles,repeats=repeats,type=type,warm=warm,present=present,lookup=lookup))
 add('duplicate-roots-and-cached-source',handles=[1,1,1],repeats=2,warm=1)
 add('failed-copy-positions',records=[source(mode=1),source(mode=2)],handles=[1,0,2,99,1,2],repeats=2)
 for a in range(5):
  for b in range(5):
   for mode in range(3):
    for extra in [0,1]:
     for lookup in [0,1]:add('name-string-copy-factory-failure',records=[source(name=a,text=b,mode=mode,extra=extra)],handles=[1,1],repeats=2,warm=1,lookup=lookup)
 for p in [0,1]:
  for t in [0xffffffff,0,14,15,0x80|15]:
   for n in [0,1,2]:add('model-attribute-gates',records=[source()]*n,handles=[0,1,2,99],present=p,type=t,repeats=2)
 for file in [0,1,0xffffffff,0xfffffffe,0x7fffffff]:add('file-detachment',records=[source(file=file),source(file=file,name=2)],handles=[2,1,2,1],repeats=2)
 rng=random.Random(2026102783)
 for _ in range(80):
  records=[source(rng.randrange(2),rng.randrange(3),rng.randrange(5),rng.getrandbits(32),rng.randrange(2),rng.randrange(5)) for _ in range(rng.randrange(3))]
  add('seeded-mixed-builder-factory',records,[rng.randrange(5) for _ in range(rng.randrange(13))],rng.randrange(1,3),rng.choice([15,15,14]),rng.randrange(2),rng.randrange(2),rng.randrange(2))
 return cases
def encode(cases):
 out=bytearray(struct.pack('<I',len(cases)))
 for c in cases:
  out+=struct.pack('<7I',len(c['records']),len(c['handles']),c['repeats'],c['type'],c['warm'],c['present'],c['lookup'])
  for r in c['records']:out+=struct.pack('<6I',*r)
  for h in c['handles']:out+=struct.pack('<I',h)
 return out
def expected(cases):
 out=bytearray(struct.pack('<I',len(cases)));counts=dict(created_roots=0,trace_events=0,resource_events=0,successful_copies=0,profile_pairs=0);witnesses=[]
 for ci,c in enumerate(cases):
  copies=[];trace=[];events=[];refs=[0,0,0];sources=[];result=[]
  for i,r in enumerate(c['records']):sources.append([i+1,i+1,r[3],r[2],0,0x205 if r[2]==4 else 0x4285,r[4],r[5] if r[4] else 4,(5 if r[5]==4 else 0x4085) if r[4] else 0])
  def acquire(e):
   if e[4]:return
   token=1 if c['lookup'] else e[3] if e[3] in [1,2] else 0
   events.append((0,token));e[4]=token
   if token:events.append((1,token));refs[token]+=1
  def snapshot():
   out.extend(struct.pack('<I',len(copies)))
   for e in copies:out.extend(struct.pack('<9I',*e))
  if c['warm']:
   for e,r in zip(sources,c['records']):
    if r[0]:acquire(e)
  for _ in range(c['repeats']):
   counts['profile_pairs']+=1;result=[];trace.append((0,900,900 if c['present'] else 0))
   if c['present'] and c['type']&31==15:
    for h in c['handles']:
     r=c['records'][h-1] if 0<h<=len(sources) else None;valid=bool(r and r[0]);trace.append((0,h,h if valid else 0));handle=0xffffffff
     if valid:
      trace.append((0,h,h))
      if r[1]==1:trace.extend([(1,h,0xffffffff),(0,0xffffffff,0)])
      else:
       new=1000+len(copies);trace.extend([(1,h,new),(0,new,0 if r[1]==2 else new)])
       if r[1]==2:e=[h,new,r[3],4,0,0x205,0,4,0]
       else:
        e=sources[h-1].copy();e[1]=new;e[2]=0xffffffff;e[4]=0;handle=new;counts['successful_copies']+=1
       copies.append(e)
     result.append(handle)
   out.extend(struct.pack('<I',len(result)))
   for h in result:out.extend(struct.pack('<I',h))
   snapshot()
   for source,h in zip(c['handles'],result):
    if 0<source<=len(sources) and c['records'][source-1][0]:acquire(sources[source-1])
    if h!=0xffffffff:acquire(copies[h-1000])
   snapshot()
  out.extend(struct.pack('<4I',*refs,len(trace)))
  for e in trace:out.extend(struct.pack('<3I',*e))
  out.extend(struct.pack('<I',len(events)))
  for e in events:out.extend(struct.pack('<2I',*e))
  counts['created_roots']+=len(copies);counts['trace_events']+=len(trace);counts['resource_events']+=len(events)
  if ci<2:witnesses.append(dict(label=c['label'],last_result=result,copies=copies,reference_counts=refs,trace=trace,resource_events=events))
 return bytes(out),counts,witnesses
def main():
 OUT.mkdir(exist_ok=True);need(sha(PARENT)=='244e69c833066823fca792ee59a88ed7b101eace66cf6a6035a0a09cfcaabfa4','accepted parent');parent=json.loads(PARENT.read_text());protected=parent['protected']|parent['source'];need(all(sha(n)==h for n,h in protected.items()),'preceding source/input identities before')
 need(sha(IFM)=='8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4','historical private IFM identity');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 _,static=check_ranges(IFM,'ifm-disassembly.txt',[('whole_backup_builder',0x10240950,0x10240b67),('whole_copy_wrapper',0x103622c0,0x1036233a),('metadata_transition',0x10361eb0,0x10361f30),('actual_material_type_check',0x105c1300,0x105c1327),('actual_factory',0x105c13b0,0x105c1416),('actual_OnConstruction',0x105c1380,0x105c13a1),('actual_string_copy',0x103667f0,0x1036680f),('actual_missing_attribute',0x103614f0,0x103615d5)])
 (run/'static-reference.json').write_text(json.dumps(static,indent=2)+'\n');sources=source_closure([Path(n) for n in parent['source']]+[ROOT/'work'/n for n in NEW]);start=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_backup_factory.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180);(run/'build.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'fresh build failure')
 exes=[]
 for tag,machine in [('reference',0x14c),('owned',0x8664)]:
  exe=OUT/('backup_factory_'+tag+('_x86.exe' if tag=='reference' else '_x64.exe'));raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];need(exe.stat().st_mtime>=start-2 and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'fresh architecture/manifest');need(not re.search(r'\b(?:warning|error)\b',(OUT/(tag+'-build.log')).read_text(),re.I),'clean build '+tag);imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(tag+'-imports.txt')).write_text(imports);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);need(modules==['KERNEL32.dll'],'system only direct imports');exes.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),imports=modules))
 cases=corpus();query=run/'query.bin';query.write_bytes(encode(cases));predicted,counts,witnesses=expected(cases);(run/'expected.bin').write_bytes(predicted);(run/'case-labels.json').write_text(json.dumps([c['label'] for c in cases],indent=2)+'\n')
 for tag,exe in zip(['reference','owned'],exes):
  target=run/(tag+'.bin');p=subprocess.run([exe['path'],str(IFM if tag=='reference' else run/'absent.dll'),str(query),str(target)],cwd=run,capture_output=True,text=True,timeout=90);(run/(tag+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,tag+' exit '+str(p.returncode)+' '+p.stderr);actual=target.read_bytes()
  if actual!=predicted:
   first=next((i for i,(a,b) in enumerate(zip(actual,predicted)) if a!=b),min(len(actual),len(predicted)));raise RuntimeError(tag+' mismatch byte '+str(first))
  need('created_roots='+str(counts['created_roots']) in p.stdout,'actual root count')
 need('profile_pairs='+str(counts['profile_pairs']) in (run/'reference.log').read_text(),'builder-only scope pairs')
 need(witnesses[0]['last_result']==[1003,1004,1005] and all(e[3]==1 and e[4]==1 for e in witnesses[0]['copies']),'independent duplicate root/resource witness')
 need(witnesses[1]['last_result']==[0xffffffff]*6,'failure position witness')
 need(all(sha(n)==h for n,h in protected.items()) and all(sha(n)==h for n,h in sources.items()),'protected sources/inputs unchanged after');need(sha(parent['editor'])==parent['editor_sha256'] and all(sha(ROOT/n)==h for n,h in parent['editor_dependencies'].items()),'editor and312deps unchanged')
 # Reuse accepted safety executable only after checking its exact identity.
 safety_exe=parent['executables'][1];need(sha(safety_exe['path'])==safety_exe['sha256'],'accepted safety executable');safety=subprocess.run([safety_exe['path'],'--self-test'],cwd=run,capture_output=True,text=True,timeout=30);(run/'binding-safety.log').write_text(safety.stdout+safety.stderr);need(safety.returncode==0 and 'checks=11' in safety.stdout,'accepted11binding safety checks')
 result=dict(status='VERIFIED joined builder/constructor string path; PARTIALLY VERIFIED full material lifecycle',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',cases=len(cases),exact_output_bytes=len(predicted),**counts,witnesses=witnesses,executables=exes,safety=safety.stdout.strip(),safety_executable=safety_exe,parent=str(PARENT),parent_sha256=sha(PARENT),source=sources,protected=protected,static_reference_sha256=sha(run/'static-reference.json'),query_sha256=sha(query),output_sha256=sha(run/'reference.bin'),editor=parent['editor'],editor_sha256=parent['editor_sha256'],editor_dependencies=parent['editor_dependencies'],editor_rebuilt=False,limits=['whole builder/copy wrapper uses actual material factory/OnConstruction/native IsA/string handler and metadata walk','DataModelcreate16 registration/dispatch and material lookup/refcounts are explicit services; GC/retirement not proven','owned string-only copy adapter is verification service, not general graph cloner','original object/attribute free lists preallocated; temporary heap and dynamic string cleanup checked separately','no production draw integration, GPU, real MaterialSystem or benchmark lifecycle'])
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','created_roots','successful_copies','trace_events','resource_events','profile_pairs','exact_output_bytes']},indent=2),flush=True)
if __name__=='__main__':main()
