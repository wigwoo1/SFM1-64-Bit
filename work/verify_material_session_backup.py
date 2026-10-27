"""Private unchanged IFM typed capture/setters versus owned64 value backup.

No SFM process, original writes, loader, instruction patches or GPU. Material
getters, canonical strings, phase, disabled undo and suppressed notifications
are explicit services. This is not whole-model capture/apply/draw/restore.
"""
import hashlib,json,random,re,struct,subprocess,tempfile,time,shutil
from datetime import datetime,timezone
from pathlib import Path
from verify_ifm_draw_packet import check_ranges
from verify_builtin_error_material_seed import source_closure
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/material-session-backup-20261027'
IFM=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/tools/ifm.dll'
MAT=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/materialsystem.dll'
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
NEW=['material_session_backup_candidate.h','probe_material_session_backup.cpp','build_material_session_backup.cmd','verify_material_session_backup.py','test_material_session_backup.cpp']
def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def need(b,s):
 if not b:raise RuntimeError(s)
def row(kind=3,before=None,live=None,name='$value',metadata=0,presence=3,state=0,old_text='old',live_text='new'):
 return dict(flags=kind|metadata,before=before if before is not None else [0]*16,live=live if live is not None else [0x3f800000]*16,name=name,presence=presence,state=state,old_text=old_text,live_text=live_text)
def corpus():
 result=[];rng=random.Random(2026102747)
 def add(rows,phase=0,flags=3):result.append((flags,phase,rows))
 for kind in range(32):
  for phase in [0,1,2,3,4,5,-1]:
   for meta in [0,0x20,0x400,0x80,0x4080,0x2080,0xffff0000]:add([row(kind=kind,metadata=meta)],phase)
 for kind in range(32):
  for name in ['$value','$FLAGS','$flags2','$flags_defined','plain','','$','a$value','$a\0suffix']:
   for presence in range(4):add([row(kind=kind,name=name,presence=presence)])
  for state in [1,2]:add([row(kind=kind,state=state)])
 special=[0,0x80000000,1,0x80000001,0x00800000,0x3f000000,0x3f800000,0xbf800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0x7fa12345,0xffc54321,0xffa54321]
 for kind in [2,3,4,8,9,10,11,14]:
  for v in special:
   for old in [v,0,0x80000000,0x7fc12345]:
    for phase in [0,3,4]:add([row(kind=kind,before=[old]*16,live=[v]*16)],phase)
 for v in range(256):
  add([row(kind=4,before=[0xdeadbe80]+[0]*15,live=[v]+[0]*15)])
  add([row(kind=8,live=[struct.unpack('<I',struct.pack('<f',v/255))[0]]*16)])
 for kind in [3,9,10,11,14]:
  # Matrix byte comparison MUST distinguish signed zero; exact NaN matrices
  # do not mutate. Vectors use ordered numeric equality instead.
  add([row(kind=kind,before=[0]*16,live=[0x80000000]*16)])
  add([row(kind=kind,before=[0x7fa12345]*16,live=[0x7fa12345]*16)])
  add([row(kind=kind,before=[0x7fc12345]*16,live=[0x7fc12345]*16)])
 for a in ['', 'old','[1 1 1]','texture/name','a\0suffix']:
  for b in ['', 'old','[1 1 1]','texture/name','a\0other']:
   for phase in [0,3,4]:add([row(kind=5,old_text=a,live_text=b)],phase)
 for _ in range(384):
  rs=[]
  for i in range(rng.randrange(1,17)):
   k=rng.randrange(32);rs.append(row(kind=k,before=[rng.getrandbits(32) for _ in range(16)],live=[rng.getrandbits(32) for _ in range(16)],metadata=rng.choice([0,0x20,0x400,0x4080]),presence=rng.randrange(4),name='$a' if i%2 else '$b'))
  add(rs,rng.choice([0,1,2,3,4,5]))
 for flags in range(4):add([row(kind=k) for k in range(32)],flags=flags)
 add([]);add([row(kind=2,name='$b'),row(kind=14,name='$a'),row(kind=5,name='$b'),row(kind=8,name='$a')]);add([row(kind=2)]*4096)
 return result
def encode(cases):
 out=bytearray(struct.pack('<I',len(cases)))
 for flags,phase,rows in cases:
  out+=struct.pack('<III',flags,phase&0xffffffff,len(rows))
  for r in rows:
   out+=struct.pack('<35I',r['flags'],r['presence'],r['state'],*r['before'],*r['live'])
   for k in ['name','old_text','live_text']:
    b=r[k].encode();out+=struct.pack('<I',len(b))+b
 return out
def quiet(v):return v|0x400000 if v&0x7fffffff>0x7f800000 else v
def same_float(a,b):return (a&0x7fffffff)<=0x7f800000 and (b&0x7fffffff)<=0x7f800000 and (a==b or not(a&0x7fffffff or b&0x7fffffff))
def color(v):
 f=struct.unpack('<f',struct.pack('<I',v))[0]
 try:x=struct.unpack('<f',struct.pack('<f',f*255.0))[0];n=int(x);return (n if -2147483648<=n<2147483648 else -2147483648)&255
 except (OverflowError,ValueError):return 0
def expected(cases):
 out=bytearray(struct.pack('<I',len(cases)));kinds={};mutations=0;events_total=0;phase_calls=0
 for ci,(flags,phase,rows) in enumerate(cases):
  out+=struct.pack('<II',ci,len(rows));events=[]
  for i,r in enumerate(rows):
   f=r['flags'];k=f&31;before=r['before'];live=r['live'];v=before.copy();text=r['old_text'].split('\0')[0];name=r['name'].split('\0')[0];different=False
   if flags&3==3 and r['state']==0 and name.startswith('$') and name.lower() not in ['$flags','$flags2']:
    events.append((0,i,0))
    if r['presence']==3:
     if k in [2,4]:events.append((1,i,0));v[0]=live[0] if k==2 else (before[0]&0xffffff00)|int(live[0]!=0);different=v[0]!=before[0]
     elif k==3:events.append((2,i,0));v[0]=quiet(live[0]);different=not same_float(v[0],before[0])
     elif k==5:events.append((5,i,0));text=r['live_text'].split('\0')[0];different=text!=r['old_text'].split('\0')[0]
     elif k==8:events.append((3,i,3));v[0]=sum(color(live[j])<<(8*j) for j in range(3));different=v[0]!=before[0]
     elif k in [9,10,11]:
      n={9:2,10:3,11:4}[k];events.append((3,i,n));different=any(not same_float(before[j],live[j]) for j in range(n));v[:n]=map(quiet,live[:n])
     elif k==14:events.append((4,i,16));different=before!=live;v=list(map(quiet,live))
   allowed=not(f&0x20) and (phase in [0,1,2] or phase==4 and not(f&0x400))
   if different and not(f&0x20):phase_calls+=1
   if different and allowed:f|=0x4080;mutations+=1
   else:v=before;text=r['old_text'].split('\0')[0]
   out+=struct.pack('<17I',f,*v);b=text.encode() if k==5 else b'';out+=struct.pack('<I',len(b))+b
  out+=struct.pack('<I',len(events))
  for event in events:out+=struct.pack('<III',*event);kinds[str(event[0])]=kinds.get(str(event[0]),0)+1
  events_total+=len(events)
 return bytes(out),dict(events=events_total,event_kinds=kinds,mutations=mutations,phase_calls=phase_calls)
def main():
 pre=json.loads((OUT/'preflight.json').read_text());need(all(sha(n)==h for n,h in pre['protected'].items()),'Protected accepted sources/original inputs before')
 need(sha(IFM)=='8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4','Historical IFM identity')
 need(sha(MAT)=='c78e414891b4d87e3922610a8bf937bca3efbaab95bced488522045998bc87f8','Historical material library identity')
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 image,evidence=check_ranges(IFM,'ifm-disassembly.txt',[
  ('whole_backup_capture',0x1023c0a0,0x1023c286),('string_intern_then_set',0x1023b3b0,0x1023b3de),('mutation_gate',0x103623b0,0x10362421),('string_set_guard',0x10365ba0,0x10365c00),('string_setter',0x10366100,0x103661c9),('matrix_guard',0x1036c800,0x1036c86e),('matrix_copy_x87',0x10496870,0x104968da),('matrix_compare_raw_bytes',0x104968e0,0x10496950),('vector2_guard',0x1036c770,0x1036c7f8)])
 need(image.word(0x10aafa60)==0x437f0000,'RGB multiplication constant255')
 mi,me=check_ranges(MAT,'materialsystem-disassembly.txt',[('actual_slot6_string_getter',0x10017de0,0x10018045)])
 need(mi.word(0x10098f34+24)==0x10017de0 and mi.word(0x10098f34+60)==0x10018220,'String getter6 distinct from texture getter15')
 (run/'static-reference.json').write_text(json.dumps(dict(ifm=evidence,material=me),indent=2)+'\n')
 parent=json.loads(Path(pre['parent']).read_text());sources=source_closure([Path(n) for n in parent['source']]+[ROOT/'work'/n for n in NEW])
 start=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_session_backup.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180);(run/'build-driver.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'Fresh x86/x64 builds')
 exes=[]
 for tag,machine in [('reference',0x14c),('owned',0x8664),('safety',0x8664)]:
  exe=OUT/('backup_'+tag+('_x86.exe' if tag=='reference' else '_x64.exe'));b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(exe.stat().st_mtime>=start-2 and struct.unpack_from('<H',b,pe+4)[0]==machine and b'asInvoker' in b,'Fresh architecture/asInvoker '+tag)
  log=(OUT/(tag+'-build.log')).read_text();need(not re.search(r'\b(?:warning|error)\b',log,re.I),'Clean compile '+tag)
  imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(tag+'-imports.txt')).write_text(imports);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);need(modules==['KERNEL32.dll'],'No engine import '+tag);exes.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),imports=modules))
 safety=subprocess.run([exes[2]['path']],cwd=run,capture_output=True,text=True,timeout=30);(run/'safety.log').write_text(safety.stdout+safety.stderr);need(safety.returncode==0 and 'backup_safety_checks=22' in safety.stdout,'22 output/rollback/phase/special-value checks')
 cases=corpus();query=run/'query.bin';query.write_bytes(encode(cases));predicted,counts=expected(cases);(run/'expected.bin').write_bytes(predicted)
 for tag,exe in zip(['reference','owned'],exes[:2]):
  target=run/(tag+'.bin');p=subprocess.run([exe['path'],str(IFM if tag=='reference' else run/'absent-engine.dll'),str(query),str(target)],cwd=run,capture_output=True,text=True,timeout=60);(run/(tag+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,tag+' child exit '+str(p.returncode));actual=target.read_bytes()
  if actual!=predicted:
   first=next((i for i,(a,b) in enumerate(zip(actual,predicted)) if a!=b),min(len(actual),len(predicted)));raise RuntimeError(tag+' mismatch at byte '+str(first)+' expected '+str(len(predicted))+' actual '+str(len(actual)))
 native_log=(run/'reference.log').read_text();need('phase_calls='+str(counts['phase_calls'])+' undo_disabled_calls='+str(counts['mutations'])+' notifications_suppressed='+str(counts['mutations']) in native_log,'Exact native mutation/service boundary counts')
 # Reuse the pinned actual-session typed query from the preceding accepted
 # adapter proof. Getter values remain explicit fixtures, never claimed live.
 from verify_material_session_override_dispatch import decode_query
 bq=Path(pre['parent']).parent/'benchmark-query.bin';need(sha(bq)==parent['benchmark_query_sha256'],'Accepted actual typed-session query')
 actual=decode_query(bq.read_bytes());benchmark=[]
 for _,attrs in actual:
  for profile in [0,1]:benchmark.append((3,0,[row(kind=r['type'],before=r['data'],live=[0x3f000000 if profile else 0]*16,name=r['name'],old_text=r['text'],live_text='controlled/backup' if profile else '') for r in attrs]))
 need(len(benchmark)==24 and sum(len(v) for _,_,v in benchmark)==1548,'Actual12snapshots in two controlled getter profiles')
 bquery=run/'benchmark-query.bin';bquery.write_bytes(encode(benchmark));bexpected,bcounts=expected(benchmark)
 for tag,exe in zip(['reference','owned'],exes[:2]):
  target=run/('benchmark-'+tag+'.bin');p=subprocess.run([exe['path'],str(IFM if tag=='reference' else run/'absent-engine.dll'),str(bquery),str(target)],cwd=run,capture_output=True,text=True,timeout=60);(run/('benchmark-'+tag+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0 and target.read_bytes()==bexpected,'Actual typed templates / controlled getters '+tag)
 need(all(sha(n)==h for n,h in sources.items()),'Entire source closure unchanged during run')
 need(all(sha(n)==h for n,h in pre['protected'].items()),'Protected accepted sources/original inputs after')
 need(sha(pre['editor'])==pre['editor_sha256'] and all(sha(ROOT/n)==h for n,h in pre['editor_dependencies'].items()),'Complete editor and312dependencies unchanged')
 result=dict(status='VERIFIED value capture only; full application INCOMPLETE',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',cases=len(cases),exact_output_bytes=len(predicted),**counts,safety=safety.stdout.strip(),benchmark_cases=len(benchmark),benchmark_attributes=sum(len(v) for _,_,v in benchmark),benchmark_events=bcounts['events'],benchmark_query_sha256=sha(bquery),benchmark_output_sha256=sha(run/'benchmark-reference.bin'),executables=exes,query_sha256=sha(query),output_sha256=sha(run/'reference.bin'),source=sources,protected=pre['protected'],parent=pre['parent'],parent_sha256=pre['parent_sha256'],static_reference=str(run/'static-reference.json'),static_reference_sha256=sha(run/'static-reference.json'),editor=pre['editor'],editor_sha256=pre['editor_sha256'],editor_dependencies=pre['editor_dependencies'],editor_rebuilt=False,limits=['Historical original capture and actual typed setters execute unchanged RX; material getter values are host-controlled, not real benchmark material state','Canonical nonnull string interning is explicit; slot6 conversion internals, null getter strings and engine symbol-pool lifetime not executed','Undo disabled and notification propagation suppressed through explicit DataModel virtual services; owner-prepare callback branch not entered','No whole model-index backup acquisition, source apply, draw/proxy or restore/lifetime transaction, production integration, GPU or visual parity','Historical IFM differs from externally modified installed IFM; no current-installation parity claim'])
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','events','event_kinds','mutations','exact_output_bytes']},indent=2),flush=True)
if __name__=='__main__':main()
