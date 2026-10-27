"""Original whole typed per-draw dispatcher vs owned64 ordered actions.

Private IFM image, unchanged executable sections; observer variable/texture
services only. No installed DLL execution, original writes, GPU or live SFM.
"""
import hashlib,json,random,re,shutil,struct,subprocess,tempfile,time
from pathlib import Path
from datetime import datetime,timezone
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges
ROOT=Path(__file__).resolve().parent.parent;OUT=ROOT/'outputs/material-session-override-dispatch-20261027'
IFM=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/tools/ifm.dll'
SESSION=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\elements\sessions\mtt_soldier\mtt_soldier.dmx')
NEW=['material_session_override_dispatch_candidate.h','session_material_override_dispatch_candidate.h','probe_material_session_override_dispatch.cpp','build_material_session_override_dispatch.cmd','verify_material_session_override_dispatch.py','test_material_session_override_dispatch.cpp']
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def need(b,s):
 if not b:raise RuntimeError(s)
def row(name='$value',kind=3,data=None,presence=3,name_state=0,string_state=0,text='models/test',texture=0x12345678):
 return dict(name=name,type=kind,data=data or [0x3f800000]*16,presence=presence,name_state=name_state,string_state=string_state,text=text,texture=texture)
def corpus():
 result=[];rng=random.Random(2026102741)
 def add(rows,flags=3):result.append((flags,rows))
 names=['$value','$flags','$FLAGS','$flags2','$FlAgS2','$flags_defined','$flags_defined2','$flags2 ','$alpha','$','$ value','plain','','$value\0suffix']
 for kind in range(32):
  for name in names:
   for presence in range(4):add([row(name,kind,presence=presence)])
 for kind in range(32):
  for upper in [0x20,0x80,0xffe0,0xffffffe0]:
   for state in range(3):add([row(kind=kind|upper,name_state=state)])
 special=[0,0x80000000,1,0x80000001,0x00800000,0x3f000000,0x3f800000,0xbf800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0x7fa12345,0xffc54321,0xffa54321]
 for kind in [2,3,4,9,10,11,14]:
  for value in special:
   for presence in range(4):add([row(kind=kind,data=[value]+[rng.getrandbits(32) for _ in range(15)],presence=presence)])
 for value in range(256):add([row(kind=4,data=[value|0xdeadbe00]+[0]*15)])
 for value in range(256):
  for alpha in [0,1,128,255]:add([row(kind=8,data=[value|((255-value)<<8)|(((value*17)&255)<<16)|(alpha<<24)]+[0]*15)])
 for _ in range(512):add([row(kind=8,data=[rng.getrandbits(32)]+[0]*15)])
 for text in ['', '1','[1 1 1]','models/a','a\0ignored','materials\\x']:
  for state in range(3):
   for texture in [0,1,0x7fffffff,0x80000000,0xffffffff]:
    for presence in range(4):add([row(kind=5,text=text,string_state=state,texture=texture,presence=presence)])
 for _ in range(128):
  rows=[row(name=rng.choice(names),kind=rng.randrange(32),data=[rng.getrandbits(32) for _ in range(16)],presence=rng.randrange(4)) for _ in range(rng.randrange(1,32))];add(rows)
 for flags in range(4):add([row(kind=t) for t in range(32)],flags)
 add([])
 # Repeated names/order cannot be reconstructed from a sorted parameter map.
 add([row('$b',2,data=[1]*16),row('$a',3),row('$b',14),row('$flags',2),row('$flags_defined',2),row('$a',5,text='[1 1 1]')])
 return result
def encoded(cases):
 out=bytearray(struct.pack('<I',len(cases)))
 for flags,rows in cases:
  out+=struct.pack('<II',flags,len(rows))
  for r in rows:
   out+=struct.pack('<21I',r['name_state'],r['string_state'],r['type'],r['presence'],r['texture'],*r['data'])
   for k in ['name','text']:
    b=r[k].encode();out+=struct.pack('<I',len(b))+b
 return out
def decode_query(data):
 at=0
 def u():
  nonlocal at
  n=struct.unpack_from('<I',data,at)[0];at+=4;return n
 cases=[]
 for _ in range(u()):
  flags=u();rows=[]
  for _ in range(u()):
   q=[u() for _ in range(21)];s=[]
   for _ in range(2):n=u();s.append(data[at:at+n].decode());at+=n
   rows.append(row(s[0],q[2],q[5:],q[3],q[0],q[1],s[1],q[4]))
  cases.append((flags,rows))
 need(at==len(data),'Query framing');return cases
def expected(cases):
 out=bytearray(struct.pack('<I',len(cases)));counts={};events_total=0
 for index,(flags,rows) in enumerate(cases):
  events=[]
  def emit(kind,i,values):
   events.append(struct.pack('<19I',kind,i,len(values),*values,*([0]*(16-len(values)))));counts[str(kind)]=counts.get(str(kind),0)+1
  if flags&3==3:
   for i,r in enumerate(rows):
    name=r['name'].split('\0')[0]
    if r['name_state'] or not name.startswith('$') or name.lower() in ['$flags','$flags2']:continue
    emit(0,i,[0,bool(r['presence']&2),bool(r['presence']&1)])
    if r['presence']!=3:continue
    kind=r['type']&31;v=r['data']
    if kind==2:emit(1,i,[v[0]])
    elif kind==3:emit(2,i,[v[0]])
    elif kind==4:emit(1,i,[bool(v[0]&255)])
    elif kind==5:
     text=None if r['string_state']==1 else '' if r['string_state']==2 else r['text'].split('\0')[0]
     emit(7,i,[text is None,text=='',0,0]);emit(8,i,[r['texture']])
    elif kind==8:
     constant=struct.unpack('<f',struct.pack('<I',0x3b808081))[0];values=[struct.unpack('<I',struct.pack('<f',((v[0]>>(8*c))&255)*constant))[0] for c in range(3)];emit(4,i,values)
    elif kind in [9,10,11]:emit({9:3,10:4,11:5}[kind],i,v[:{9:2,10:3,11:4}[kind]])
    elif kind==14:emit(6,i,v)
  out+=struct.pack('<II',index,len(events))+b''.join(events);events_total+=len(events)
 return bytes(out),counts,events_total
def main():
 OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 pointer=json.loads((ROOT/'outputs/scene-material-parameter-bridge-20261027/latest-verification.json').read_text());parent=Path(pointer['receipt']);need(sha(parent)==pointer['receipt_sha256']=='7ace2bb8d8fb4c117ed78b5b0f15f3613c493e886be3b370dd2249a015e02356','Accepted actual-material checkpoint');prior=json.loads(parent.read_text())
 need(sha(IFM)=='8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4','Historical private IFM image, not installed patched DLL')
 protected={**prior['source'],**prior['original_readonly_hashes']};need(all(sha(n)==h for n,h in protected.items()),'Protected inputs before')
 sources=source_closure([Path(n) for n in prior['source']]+[ROOT/'work'/n for n in NEW])
 ranges=[('complete_per_draw_typed_override_application',0x1023e140,0x1023e3a4),('first_attribute_accessor',0x1012c420,0x1012c424),('material_cached_get_or_find',0x105c1220,0x105c1260),('material_reference_assignment',0x10984a60,0x10984a9f),('complete_source_or_backup_apply_loop',0x1023e3e0,0x1023e4b4),('draw_override_apply_before_packet_static',0x10243861,0x102438c2),('draw_backup_restore_call_static',0x10243fc4,0x10243fe3),('byte_case_insensitive_compare',0x109751a0,0x1097521e),('matrix_identity_initializer',0x10496fc0,0x10497022)]
 image,evidence=check_ranges(IFM,'ifm-disassembly.txt',ranges);(run/'static-reference.json').write_text(json.dumps(evidence,indent=2)+'\n')
 need(image.string(0x10adb748)=='$flags' and image.string(0x10adb740)=='$flags2' and image.word(0x10ac2a30)==0x3b808081,'Exact excluded names and color scaling constant')
 table=[image.word(0x1023e3a4+4*i) for i in range(13)];need(table==[0x1023e217,0x1023e22a,0x1023e1fe,0x1023e35b,0x1023e393,0x1023e393,0x1023e2ee,0x1023e249,0x1023e275,0x1023e2ac,0x1023e393,0x1023e393,0x1023e348],'Actual complete switch table')
 start=time.time();build=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_session_override_dispatch.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=240);(run/'build-driver.log').write_text(build.stdout+build.stderr);need(build.returncode==0,'Fresh reference/owned builds')
 executables=[]
 for name,machine,tag in [('override_reference_x86',0x14c,'reference'),('override_owned_x64',0x8664,'owned'),('override_safety_x64',0x8664,'safety')]:
  exe=OUT/(name+'.exe');b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(exe.stat().st_mtime>=start-2 and struct.unpack_from('<H',b,pe+4)[0]==machine and b'asInvoker' in b,'Fresh expected architecture/asInvoker')
  imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(name+'-imports.txt')).write_text(imports);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);need(modules==['KERNEL32.dll'],'Direct system imports only')
  log=(OUT/(tag+'-build.log')).read_text();warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',log);need(all(c=='C4458' for c,_ in warnings),'Only existing hierarchy warning');executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),imports=modules,warnings=warnings))
 def execute(exe,args,label):
  p=subprocess.run([str(OUT/(exe+'.exe')),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=180);(run/(label+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,label+' exit '+str(p.returncode));return p.stdout
 safety=execute('override_safety_x64',[],'owned-safety');need('typed_override_safety_checks=19' in safety,'19 typed adapter/service safety checks')
 cases=corpus();query=run/'query.bin';query.write_bytes(encoded(cases));need(decode_query(query.read_bytes())==cases,'Independent query roundtrip')
 predicted,counts,total=expected(cases);need(all(counts.get(str(k),0)>=32 for k in range(9)),'Every observed setter/lookup path has at least32 cases')
 original=run/'native.bin';owned=run/'owned.bin';execute('override_reference_x86',[IFM,query,original],'original');execute('override_owned_x64',[run/'absent-engine.dll',query,owned],'owned');need(original.read_bytes()==predicted,'Whole original dispatcher matches independent full action trace');need(owned.read_bytes()==predicted,'Owned64 matches every original event word/order')
 benchmark=run/'benchmark-query.bin';benchmark_log=execute('override_owned_x64',['--benchmark',SESSION,benchmark],'benchmark-query');actual=decode_query(benchmark.read_bytes());need(len(actual)==12 and len(re.findall(r'^benchmark_case=',benchmark_log,re.M))==12,'Four actual DmeMaterial override records across three frames, including inactive material names')
 original_benchmark=run/'benchmark-native.bin';owned_benchmark=run/'benchmark-owned.bin';execute('override_reference_x86',[IFM,benchmark,original_benchmark],'benchmark-original');execute('override_owned_x64',[run/'absent-engine.dll',benchmark,owned_benchmark],'benchmark-owned');benchmark_expected,benchmark_counts,benchmark_total=expected(actual);need(original_benchmark.read_bytes()==owned_benchmark.read_bytes()==benchmark_expected,'Actual session typed attrs match original dispatch with explicit controlled variable/texture services')
 need(all(sha(n)==h for n,h in protected.items()) and all(sha(n)==h for n,h in sources.items()),'Inputs/source unchanged after reference children')
 default=Path(prior['editor_executable']);need(sha(default)==prior['editor_sha256'] and all(sha(ROOT/n)==h for n,h in prior['editor_dependencies'].items()),'Complete editor and312dependencies unchanged')
 result=dict(status='PASS typed per-draw dispatch prerequisite; full application incomplete',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',cases=len(cases),events=total,event_kind_counts=counts,exact_trace_bytes=len(predicted),query_sha256=sha(query),output_sha256=sha(original),safety=safety.strip(),benchmark_material_snapshots=len(actual),benchmark_attributes=sum(len(v) for _,v in actual),benchmark_events=benchmark_total,benchmark_event_counts=benchmark_counts,benchmark_query_sha256=sha(benchmark),benchmark_output_sha256=sha(original_benchmark),executables=executables,static_reference=str(run/'static-reference.json'),static_reference_sha256=sha(run/'static-reference.json'),source=sources,protected=protected,parent_receipt=str(parent),parent_receipt_sha256=sha(parent),editor_executable=str(default),editor_sha256=sha(default),editor_dependencies=prior['editor_dependencies'],editor_rebuilt=False,limits=['Original whole dispatcher and actual typed getters execute unchanged RX; material FindVar, texture lookup and typed setters are explicit boundary observers','Historical private IFM differs from changed installed IFM; no current-installation parity claim','Outer acquisition/backup/apply/restore ordering is static verified evidence, not whole DrawModel or combined proxy/texture/resource execution','Resource texture return is an explicit semantic fixture token, including actual null; no texture loader or owning resource implemented','Real benchmark supplies owned Graph attribute order only; native loaded-element chain order, target variable presence and texture return services are not reconstructed','Owned adapter/dispatcher not connected to production material store, UI or GPU; source/backup scopes and full atomic publication remain to implement'])
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','events','event_kind_counts','benchmark_material_snapshots','benchmark_attributes','benchmark_events']},indent=2))
if __name__=='__main__':main()
