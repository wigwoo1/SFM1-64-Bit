"""Real owned IDs/refcounts vs native element cache protocol; loose VMT audit."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,re,struct,subprocess,tempfile,time
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges
from verify_material_element_binding import corpus,expected
ROOT=Path(__file__).resolve().parent.parent;OUT=ROOT/'outputs/material-store-bindings-20261027'
IFM=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/tools/ifm.dll'
GAME=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game');BENCH=GAME/'tf_movies/elements/sessions/mtt_soldier/mtt_soldier.dmx'
PRIOR=ROOT/'outputs/material-find-cached-20261027/verification-aihox4s9/benchmark-audit.json'
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
NEW=['material_session_store_bindings_candidate.h','test_material_store_bindings.cpp','probe_material_store_bindings_benchmark.cpp','build_material_store_bindings.cmd','verify_material_store_bindings.py']
def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def need(b,s):
 if not b:raise RuntimeError(s)
def projection(raw,count):
 at=0
 def words(n):
  nonlocal at
  need(at+4*n<=len(raw),'native bounds');v=list(struct.unpack_from('<'+'I'*n,raw,at));at+=4*n;return v
 need(words(1)==[count],'case count');out=bytearray(struct.pack('<I',count));event_count=0
 for _ in range(count):
  for _ in range(10):out+=struct.pack('<3I',*words(5)[:3])
  out+=struct.pack('<3I',*words(3));n=words(1)[0];counts=[0,0,0]
  for _ in range(n):kind,resource=words(2);need(kind<3 and resource<=2,'reference event');counts[kind]+=1
  out+=struct.pack('<3I',*counts);event_count+=n
 need(at==len(raw),'native tail');return bytes(out),event_count
def main():
 pre=json.loads((OUT/'preflight.json').read_text());parent=json.loads(Path(pre['parent']).read_text())
 need(sha(pre['parent'])==pre['parent_sha256']=='5a0cf1f03c527d9f222187e659770b0e186095660ce3c41c4124004fed04fad7','accepted parent')
 need(all(sha(k)==v for k,v in pre['protected'].items()),'all preceding source/inputs before')
 need(sha(IFM)=='8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4' and sha(BENCH)=='5a08cf8eeb086586f05dc856fb5469dd238fde90315ecf17c2dd4ab5f0742aee','original input identities')
 sources=source_closure([Path(k) for k in parent['source']]+[ROOT/'work'/k for k in NEW]+[ROOT/'work/probe_material_element_binding.cpp']);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 _,ranges=check_ranges(IFM,'ifm-disassembly.txt',[('name_set',0x105c11c0,0x105c11ef),('resolve',0x105c11f0,0x105c1209),('resource_get',0x105c1220,0x105c1260),('cache_release',0x10984960,0x10984999),('cache_assign',0x10984a60,0x10984a9f)])
 (run/'static-reference.json').write_text(json.dumps(ranges,indent=2));started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_store_bindings.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=240);(run/'build-driver.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'fresh eight builds')
 exes={}
 for tag,name,machine in [('reference','bindings_reference_x86',0x14c),('owned','bindings_owned_x64',0x8664),('batch','batch_regression_x64',0x8664),('single','single_regression_x64',0x8664),('store','store_regression_x64',0x8664),('parameters','parameters_regression_x64',0x8664),('cached','cached_regression_x64',0x8664),('benchmark','bindings_benchmark_x64',0x8664)]:
  exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];need(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'fresh architecture/manifest '+tag)
  log=(OUT/(tag+'-build.log')).read_text();(run/(tag+'-build.log')).write_text(log);need(not re.search(r'\b(?:warning|error)\b',log,re.I),'clean compile '+tag)
  inspection=subprocess.run([str(DUMP),'/headers','/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(tag+'-inspection.txt')).write_text(inspection);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',inspection,re.M|re.I);need(modules==['KERNEL32.dll'],'system-only direct imports '+tag);exes[tag]=dict(path=str(exe),sha256=sha(exe),machine=hex(machine),imports=modules)
 def execute(tag,args,label=None):
  p=subprocess.run([exes[tag]['path']]+list(map(str,args)),cwd=run,capture_output=True,text=True,timeout=180);(run/((label or tag)+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,tag+' failed: '+p.stderr);return p.stdout.strip()
 safety=execute('owned',[],'safety');counts=re.search(r'checks=(\d+) allocation_failures=(\d+)',safety);need(counts and int(counts[2])>100,'binding lifetime/allocation safety')
 regressions={tag:execute(tag,['--test'] if tag in ['store','parameters'] else []) for tag in ['batch','single','store','parameters','cached']}
 for tag,needle in [('batch','checks=1577 actual_allocation_failures=1519'),('single','checks=888 actual_allocation_failures=788'),('store','checks=4657'),('parameters','checks=281'),('cached','checks=20 plus_prior=49')]:need(needle in regressions[tag],'existing regression '+tag)
 cases=corpus();query=run/'query.bin';query.write_bytes(struct.pack('<I',len(cases))+b''.join(struct.pack('<7I',*q) for q in cases));predicted,total,witnesses=expected(cases)
 execute('reference',[IFM,query,run/'native.bin']);need((run/'native.bin').read_bytes()==predicted,'entire original name/member copy/resolve/acquire trace vs independent model')
 canonical,event_count=projection((run/'native.bin').read_bytes(),len(cases));need(event_count==total,'native event count');(run/'native-projected.bin').write_bytes(canonical)
 execute('owned',['--wire',query,run/'owned.bin'],'owned-wire');need((run/'owned.bin').read_bytes()==canonical,'actual owned store cache/flags/name/refcounts vs original projection')
 benchmark=execute('benchmark',[BENCH,GAME,run/'benchmark.tsv']);old=json.loads(PRIOR.read_text());materials={};owners={};bindings={};objects={};retired={};batches=[];files={}
 for line in (run/'benchmark.tsv').read_text().splitlines():
  v=line.split('\t')
  if v[0]=='M':materials[v[1]]={'name':v[2],'key':v[3]};bindings[v[1]]=dict(available=int(v[4]),resource=int(v[5]),reason=v[6])
  elif v[0]=='O':owners.setdefault(v[1],[]).append(dict(position=int(v[2]),material=int(v[3])))
  elif v[0]=='S':objects[int(v[1])]=dict(name=v[2],initialized=int(v[3]),references=int(v[4]),shader=v[5] if len(v)>5 else '')
  elif v[0]=='R':retired[int(v[1])]=int(v[2])
  elif v[0]=='B':batches.append(list(map(int,v[1:])))
  elif v[0]=='F':
   path=Path(v[1]);data=path.read_bytes();h=14695981039346656037
   for byte in data:h=((h^byte)*1099511628211)&0xffffffffffffffff
   need(len(data)==int(v[2]) and h==int(v[3]),'file bytes unchanged since read-only audit '+str(path));files[str(path)]=hashlib.sha256(data).hexdigest()
 need(materials==old['materials'] and owners==old['owners'],'benchmark names/positions unchanged')
 need(len(materials)==236 and len(batches)==107 and sum(x[1] for x in batches)==211,'benchmark total schema')
 expected_refs={k:0 for k in objects}
 for b in bindings.values():
  if b['available']:
   need(b['resource'] in objects,'acquired actual owned resource ID; initialization checked separately');expected_refs[b['resource']]+=1
  else:need(b['resource']==0,'unavailable result not cached')
 need(all(o['references']==expected_refs[k] for k,o in objects.items()),'actual refcount one per cached element, not position/name')
 need(set(retired)==set(objects) and all(v==0 for v in retired.values()),'every benchmark resource reference released; objects remain owned cache')
 need(all((b[2]==3 and b[3]==1) or (b[2] in [1,2] and b[3]==0) for b in batches),'no partial observer on unsupported binding/value')
 ready=sum(b['available'] for b in bindings.values());need(ready>0,'some real loose VMT resource objects must bind');initialized=sum(o['initialized'] for o in objects.values())
 need(all(sha(k)==v for k,v in files.items()) and all(sha(k)==v for k,v in pre['protected'].items()) and all(sha(k)==v for k,v in sources.items()),'all read inputs/prior/new sources unchanged after')
 need(sha(pre['editor'])==pre['editor_sha256'] and all(sha(ROOT/k)==v for k,v in pre['editor_dependencies'].items()),'editor313 dependencies unchanged')
 audit=dict(materials=materials,owners=owners,bindings=bindings,objects=objects,batches=batches,read_inputs=files);(run/'benchmark-audit.json').write_text(json.dumps(audit,indent=2))
 result=dict(status='VERIFIED owned session-element resource lifetime and bounded native protocol comparison; application PARTIALLY VERIFIED',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',binding_checks=int(counts[1]),allocation_failures=int(counts[2]),wire_cases=len(cases),native_events=total,native_bytes=len(predicted),projected_bytes=len(canonical),projected_sha256=sha(run/'native-projected.bin'),safety=safety,regressions=regressions,benchmark=benchmark,benchmark_counts=dict(elements=236,bound_resources=ready,initialized_cpu=initialized,unavailable=236-ready,cached_objects=len(objects),arrays=107,positions=211,observed=sum(b[2]==3 for b in batches),unavailable_batches=sum(b[2]==1 for b in batches),value_refused=sum(b[2]==2 for b in batches),retained_references=sum(expected_refs.values()),released_references=sum(expected_refs.values()),read_files=len(files)),benchmark_audit_sha256=sha(run/'benchmark-audit.json'),read_inputs=files,source=sources,protected=pre['protected'],parent=pre['parent'],parent_sha256=pre['parent_sha256'],executables=exes,editor=pre['editor'],editor_sha256=pre['editor_sha256'],editor_dependencies=pre['editor_dependencies'],editor_rebuilt=False,limits=['New owner-thread bridge owns one actual store reference per session element cache; repeated element acquire does not retain twice; distinct elements sharing ID retain independently','Acquisition/cache/loader effects persist separately from failed parameter rollback; late unavailable lookup or value refusal keeps earlier acquisitions until explicit release','Cached-only default selects filename category0 and refuses unavailable miss; optional full lookup returns borrowed actual ID (initialization separate), explicit known0 or nullopt unavailable','Native unchanged RX factory/member/string-copy/name-set/resolve/acquire trace uses explicit resource/type/heap services; comparison selects names/flags/cache identity/refcounts and event counts, not a full original MaterialSystem lifetime or Graph clone','Read-only benchmark provider loads loose VMTs through declared GameInfo mount order and existing owned CPU initialization. VPK/dynamic mounts/GPU resources/textures/proxy factory/native resource parity are not implemented','Direct reference edits/forced removal and owner destruction in callbacks violate retention contract; stale IDs refuse, destructor never fabricates successful release','No original writes/live SFM/computer use/editor launch/GPU/document-history/serialization changes/dependency install/publication'])
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','binding_checks','allocation_failures','wire_cases','native_events','projected_bytes','benchmark_counts']},indent=2),flush=True)
if __name__=='__main__':main()
