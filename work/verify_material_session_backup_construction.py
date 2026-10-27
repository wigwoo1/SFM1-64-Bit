"""Original whole backup builder/copy wrappers versus owned collection policy.

Factory-preseeded leaf attributes and value-copy VM services are explicit.
No real SFM process, import loader, instruction patches, originals writes or GPU.
Does not establish missing-attribute creation, reference graph cloning or the
real DmeMaterial factory/mtlName cache invariant needed for production drawing.
"""
import hashlib,json,random,re,struct,subprocess,tempfile,time
from pathlib import Path
from datetime import datetime,timezone
from verify_ifm_draw_packet import check_ranges
from verify_builtin_error_material_seed import source_closure
ROOT=Path(__file__).resolve().parent.parent;OUT=ROOT/'outputs/material-session-backup-construction-20261027'
IFM=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/tools/ifm.dll'
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
NEW=['material_session_backup_construction_candidate.h','probe_material_session_backup_construction.cpp','build_material_session_backup_construction.cmd','test_material_session_backup_construction.cpp','verify_material_session_backup_construction.py']
TYPES={2:4,3:4,4:1,8:4,9:8,10:12,11:16,14:64}
def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def need(b,s):
 if not b:raise RuntimeError(s)
def source(available=1,base=1,material=1,factory=0,file=123,kind=2,flags=0,attribute=1,words=None):return [available,base,material,factory,file,kind,flags,attribute,*([0x3f800000]*16 if words is None else words)]
def corpus():
 result=[];rng=random.Random(2026102767)
 def add(label,records=None,handles=None,present=1,attribute=15,repeats=1):result.append(dict(label=label,records=[source()] if records is None else records,handles=[1] if handles is None else handles,present=present,attribute=attribute,repeats=repeats))
 add('duplicate-source-independent-clones',handles=[1,1,1],repeats=2)
 add('invalid-placeholders-preserve-indices',records=[source(),source(available=0),source(base=0),source(factory=1),source(factory=2),source(factory=3),source(factory=4),source(material=0)],handles=[1,0,99,2,3,4,5,6,7,8,1])
 for present in [0,1]:
  for attribute in [0xffffffff,0,1,14,15,16,0x80|15,0x4000|15]:
   for handles in [[],[0],[1],[1,1,99,0]]:add('model-materials-attribute-gate',handles=handles,present=present,attribute=attribute,repeats=2)
 for available in [0,1]:
  for base in [0,1]:
   for material in [0,1]:
    for factory in range(5):
     for file in [0,1,0xffffffff,0xfffffffe,0x7fffffff]:
      for attr in [0,1]:add('type-factory-file-branch',records=[source(available,base,material,factory,file,attribute=attr)],repeats=2)
 for kind in TYPES:
  for flags in [0,0x20,0x80,0x200,0x400,0x800,0x1000,0x2000,0x4080,0xffe0]:
   for word in [0,0x80000000,1,0x3f800000,0x7fc12345,0x7fa12345,0xffffffff]:add('typed-existing-target-copy',records=[source(kind=kind,flags=flags,words=[word]*16)],handles=[1,1],repeats=2)
 for _ in range(128):
  n=rng.randrange(1,17);records=[source(rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.randrange(5),rng.getrandbits(32),rng.choice(list(TYPES)),rng.choice([0,0x20,0x4080]),rng.randrange(2),[rng.getrandbits(32) for _ in range(16)]) for _ in range(n)]
  add('seeded-position-and-factory-policy',records,[rng.randrange(n+3) for _ in range(rng.randrange(33))],rng.randrange(2),rng.choice([15,15,0xffffffff,14]),rng.randrange(1,3))
 add('maximum-controlled-chain',records=[source(kind=list(TYPES)[i%8]) for i in range(64)],handles=list(range(1,65)),repeats=2)
 return result
def encode(cases):
 out=bytearray(struct.pack('<I',len(cases)))
 for q in cases:
  out+=struct.pack('<5I',q['present'],q['attribute'],len(q['handles']),len(q['records']),q['repeats'])
  for r in q['records']:out+=struct.pack('<24I',*r)
  out+=struct.pack('<'+'I'*len(q['handles']),*q['handles'])
 return out
def expected(cases):
 out=bytearray(struct.pack('<I',len(cases)));kinds={};value_copies=0;scope_pairs=0;witnesses=[]
 for ci,q in enumerate(cases):
  clones=[];events=[];result=[]
  for _ in range(q['repeats']):
   scope_pairs+=1;result=[];events.append((0,900,900 if q['present'] else 0,0))
   if not q['present'] or q['attribute']&31!=15:continue
   for h in q['handles']:
    r=q['records'][h-1] if 0<h<=len(q['records']) else None;resolved=h if r and r[0] else 0;events.append((0,h,resolved,0));target=0xffffffff
    if resolved:
     events.append((1,h,r[1],0))
     if r[1]:
      events.extend([(0,h,h,0),(1,h,r[1],0)])
      if r[3]==1:events.append((2,h,0xffffffff,r[3]))
      else:
       handle=1000+len(clones);flags=r[5]|r[6];words=[0]*16
       c=[h,handle,int(r[3]!=3),int(bool(r[2]) and r[3]!=4),r[4],r[5],flags,r[7],*words];clones.append(c);events.append((2,h,handle,r[3]))
       if r[3] not in [2,3]:
        if r[7]:
         raw=struct.pack('<16I',*r[8:]);copied=raw[:TYPES[r[5]]]+bytes(64-TYPES[r[5]]);c[8:]=struct.unpack('<16I',copied);value_copies+=1
        events.append((3,handle,c[3],0))
        if c[3]:c[4]=0xffffffff;target=handle
    result.append(target)
  out+=struct.pack('<II',ci,len(result))+struct.pack('<'+'I'*len(result),*result)+struct.pack('<I',len(clones))
  for c in clones:out+=struct.pack('<24I',*c)
  out+=struct.pack('<I',len(events))
  for e in events:out+=struct.pack('<4I',*e);kinds[str(e[0])]=kinds.get(str(e[0]),0)+1
  if ci<2:witnesses.append(dict(label=q['label'],result=result,created_roots=len(clones),clone_source_handles=[c[0] for c in clones],clone_file_ids=[hex(c[4]) for c in clones]))
 return bytes(out),dict(events=sum(kinds.values()),event_kinds=kinds,value_copy_services=value_copies,scope_pairs=scope_pairs,witnesses=witnesses)
def main():
 pre=json.loads((OUT/'preflight.json').read_text());need(sha(pre['parent'])==pre['parent_sha256']=='a0268685d930e640abf6c89bf42bbbe67fa633b4cfe6f9ed4827365a111e9909','Accepted collection receipt');need(all(sha(n)==h for n,h in pre['protected'].items()),'Protected preceding sources and inputs before')
 need(sha(IFM)=='8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4','Historical private image identity')
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 image,static=check_ranges(IFM,'ifm-disassembly.txt',[
  ('whole_backup_builder',0x10240950,0x10240b67),('builder_initialization_call',0x102423eb,0x10242409),('secondary_draw_this_adjustment',0x102437e4,0x102437f4),('draw_backup_base',0x10243891,0x102438c2),('whole_copy_wrapper',0x103622c0,0x1036233a),('whole_attribute_copy_traversal',0x10361d30,0x10361ead),('whole_copy_context',0x103621f0,0x103622b5),('whole_file_transition_wrapper',0x10361eb0,0x10361f30),('typed_value_copy_dispatch',0x103629a0,0x103629d2),('element_handle_accessor',0x10335830,0x10335834),('constructor_vector_zeroing',0x1024329f,0x102432db)])
 need(image.string(0x10aa68d4)=='materials','Actual attribute name')
 (run/'static-reference.json').write_text(json.dumps(static,indent=2)+'\n');parent=json.loads(Path(pre['parent']).read_text());sources=source_closure([Path(n) for n in parent['source']]+[ROOT/'work'/n for n in NEW])
 start=time.time();build=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_session_backup_construction.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180);(run/'build-driver.log').write_text(build.stdout+build.stderr);need(build.returncode==0,'Fresh reference/owned/safety builds')
 exes=[]
 for tag,machine in [('reference',0x14c),('owned',0x8664),('safety',0x8664)]:
  exe=OUT/('backup_construction_'+tag+('_x86.exe' if tag=='reference' else '_x64.exe'));b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(exe.stat().st_mtime>=start-2 and struct.unpack_from('<H',b,pe+4)[0]==machine and b'asInvoker' in b,'Fresh binary architecture/asInvoker '+tag)
  log=(OUT/(tag+'-build.log')).read_text();need(not re.search(r'\b(?:warning|error)\b',log,re.I),'Clean compile '+tag)
  imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(tag+'-imports.txt')).write_text(imports);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);need(modules==['KERNEL32.dll'],'System-only direct imports '+tag);exes.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),imports=modules))
 cases=corpus();query=run/'query.bin';query.write_bytes(encode(cases));predicted,counts=expected(cases);(run/'expected.bin').write_bytes(predicted);(run/'case-labels.json').write_text(json.dumps([q['label'] for q in cases],indent=2)+'\n')
 need(counts['witnesses'][0]['created_roots']==6 and counts['witnesses'][0]['result']==[1003,1004,1005],'Repeated handles independently copied in repeated builds')
 need(len(counts['witnesses'][1]['result'])==11 and counts['witnesses'][1]['result'][1:10]==[0xffffffff]*9,'Failure placeholders preserve source positions')
 for tag,exe in zip(['reference','owned'],exes[:2]):
  target=run/(tag+'.bin');p=subprocess.run([exe['path'],str(IFM if tag=='reference' else run/'absent-engine.dll'),str(query),str(target)],cwd=run,capture_output=True,text=True,timeout=90);(run/(tag+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,tag+' child exit '+str(p.returncode));actual=target.read_bytes()
  if actual!=predicted:
   first=next((i for i,(a,b) in enumerate(zip(actual,predicted)) if a!=b),min(len(actual),len(predicted)));raise RuntimeError(tag+' mismatch byte '+str(first)+' expected '+str(len(predicted))+' actual '+str(len(actual)))
 native=(run/'reference.log').read_text();need('actual_copy_value_services='+str(counts['value_copy_services'])+' scope_pairs='+str(counts['scope_pairs']) in native,'Actual native leaf-copy service and scope counts')
 safety=subprocess.run([exes[2]['path']],cwd=run,capture_output=True,text=True,timeout=30);(run/'safety.log').write_text(safety.stdout+safety.stderr);need(safety.returncode==0 and 'backup_construction_safety_checks=14' in safety.stdout,'14 staged-output/service safety checks')
 need(all(sha(n)==h for n,h in pre['protected'].items()) and all(sha(n)==h for n,h in sources.items()),'Protected inputs and435preceding sources unchanged after')
 need(sha(pre['editor'])==pre['editor_sha256'] and all(sha(ROOT/n)==h for n,h in pre['editor_dependencies'].items()),'Complete editor and312deps unchanged')
 result=dict(status='VERIFIED construction policy / PARTIALLY VERIFIED full clone',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',cases=len(cases),exact_output_bytes=len(predicted),**counts,safety=safety.stdout.strip(),executables=exes,query_sha256=sha(query),output_sha256=sha(run/'reference.bin'),source=sources,protected=pre['protected'],parent=pre['parent'],parent_sha256=pre['parent_sha256'],static_reference=str(run/'static-reference.json'),static_reference_sha256=sha(run/'static-reference.json'),editor=pre['editor'],editor_sha256=pre['editor_sha256'],editor_dependencies=pre['editor_dependencies'],editor_rebuilt=False,limits=['Whole original builder, clone wrapper/copy context/attribute traversal, temporary containers, and metadata transition execute unchanged RX','Factory creates preseeded matching leaf attrs; typed value copy is an explicit virtual host service, not the real DmeMaterial constructor or original typed value implementation','Missing/new attributes, actual mtlName/cache propagation, reference graph cloning and runtime attribute chain order remain unverified','Owner/file registry is empty in the reference service; +38 transition is verified, real file accounting/element retirement/GC is not','No real material resolution, combined capture/apply/draw/restore, GPU/visual or real benchmark lifecycle claim','Historical reference differs from modified installed IFM; originals read-only, no live SFM/loader/instruction patches'])
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','events','exact_output_bytes','value_copy_services','scope_pairs','witnesses']},indent=2),flush=True)
if __name__=='__main__':main()
