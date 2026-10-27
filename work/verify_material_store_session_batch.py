"""Actual owned material batch scope vs original collection order.

Native host getter/setter services are integer-only controlled witnesses here.
Benchmark audit refuses unavailable bindings; it does not invent loaded materials.
"""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,random,re,struct,subprocess,tempfile,time
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges
from verify_material_session_collection import case,element,attr,encode,expected

ROOT=Path(__file__).resolve().parent.parent;OUT=ROOT/'outputs/material-store-session-batch-20261027'
IFM=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/tools/ifm.dll'
BENCH=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\elements\sessions\mtt_soldier\mtt_soldier.dmx')
PRIOR=ROOT/'outputs/material-find-cached-20261027/verification-aihox4s9/benchmark-audit.json'
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
NEW=['test_material_store_session_batch.cpp','probe_material_store_batch_benchmark.cpp','build_material_store_session_batch.cmd','verify_material_store_session_batch.py']
def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def need(b,s):
 if not b:raise RuntimeError(s)
def corpus():
 out=[[(1,11),(1,22)],[],[(0,123),(0,456)],[(2,11),(1,33),(2,22)],[(1,11),(2,33),(1,22)],[(1,0xffffffff)],[(2,0x80000000)]]
 rng=random.Random(2026102764)
 for _ in range(512):out.append([(rng.randrange(3),rng.getrandbits(32)) for _ in range(rng.randrange(33))])
 return out
def native_case(entries):
 sources=[element([attr(2,words=[v]+[0]*15)],cached=t,lookup=t,available=int(bool(t))) for t,v in entries]
 backups=[element([attr(2,words=[0]*16)],cached=t,lookup=t,name_state=0 if t else 1) for t,_ in entries]
 n=len(entries);return case('ordered-owned-ID-aliases',sources+backups,list(range(1,n+1)),list(range(n+1,2*n+1)))
def independent(cases):
 out=bytearray(struct.pack('<I',len(cases)))
 for ci,entries in enumerate(cases):
  values={1:7,2:9};backups=[]
  for target,value in entries:
   backups.append(values[target] if target else 0)
   if target:values[target]=value
  applied=[values[1],values[2]]
  for (target,_),backup in zip(entries,backups):
   if target:values[target]=backup
  out+=struct.pack('<'+'I'*(6+len(entries)),ci,len(entries),*applied,*backups,values[1],values[2])
 return bytes(out)
def project_native(raw,cases):
 at=0
 def word():
  nonlocal at
  need(at+4<=len(raw),'native word bounds');v=struct.unpack_from('<I',raw,at)[0];at+=4;return v
 def words(n):return [word() for _ in range(n)]
 def text():
  nonlocal at
  n=word();need(at+n<=len(raw),'native text bounds');at+=n
 need(word()==len(cases),'native case count');out=bytearray(struct.pack('<I',len(cases)))
 for ci,q in enumerate(cases):
  need(word()==ci and word()==2,'native two ordered phases');snapshots=[]
  for _ in range(2):
   mats=[];els=[]
   for m in q['materials']:
    word();vs=[]
    for _ in m['vars']:vs.append(words(16));text()
    mats.append(vs)
   for e in q['elements']:
    word();attrs=[]
    for _ in e['attrs']:word();attrs.append(words(16));text()
    els.append(attrs)
   snapshots.append((mats,els))
  events=word();need(at+events*80<=len(raw),'native event bounds');at+=events*80
  n=len(q['source']);applied,restored=snapshots
  backups=[applied[1][n+i][0][0] for i in range(n)]
  out+=struct.pack('<'+'I'*(6+n),ci,n,applied[0][0][0][0],applied[0][1][0][0],*backups,restored[0][0][0][0],restored[0][1][0][0])
 need(at==len(raw),'native output tail');return bytes(out)
def main():
 pre=json.loads((OUT/'preflight.json').read_text());parent=json.loads(Path(pre['parent']).read_text())
 need(sha(pre['parent'])==pre['parent_sha256']=='852777019c97711498cc6e9729c863a395066143967d1b3c61a3f236a8411ce2','parent identity')
 protected={k:v for k,v in pre['protected'].items() if k not in pre['intended_changes']}
 need(all(sha(k)==v for k,v in protected.items()),'unrelated protected inputs before')
 need(sha(OUT/'before/material_object_store_candidate.h')==parent['source'][str(ROOT/'work/material_object_store_candidate.h')],'saved old store identity')
 need(sha(IFM)=='8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4','historical IFM')
 need(sha(BENCH)=='5a08cf8eeb086586f05dc856fb5469dd238fde90315ecf17c2dd4ab5f0742aee','read-only benchmark identity')
 sources=source_closure([Path(k) for k in parent['source']]+[ROOT/'work'/k for k in NEW]+[ROOT/'work/probe_material_session_collection.cpp'])
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 _,ranges=check_ranges(IFM,'ifm-disassembly.txt',[('whole_collection',0x1023e3e0,0x1023e4b4),('whole_capture',0x1023c0a0,0x1023c286),('whole_apply',0x1023e140,0x1023e3a4),('lazy_resource',0x105c1220,0x105c1260)])
 (run/'static-reference.json').write_text(json.dumps(ranges,indent=2))
 started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_store_session_batch.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=240);(run/'build-driver.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'fresh seven executable builds')
 exes={}
 for tag,name,machine in [('reference','batch_reference_x86',0x14c),('owned','batch_owned_x64',0x8664),('single','single_regression_x64',0x8664),('store','store_regression_x64',0x8664),('parameters','parameters_regression_x64',0x8664),('cached','cached_regression_x64',0x8664),('benchmark','benchmark_mapping_x64',0x8664)]:
  exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
  need(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'fresh architecture/manifest '+tag)
  log=(OUT/(tag+'-build.log')).read_text();(run/(tag+'-build.log')).write_text(log);need(not re.search(r'\b(?:warning|error)\b',log,re.I),'clean compile '+tag)
  inspection=subprocess.run([str(DUMP),'/headers','/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(tag+'-inspection.txt')).write_text(inspection);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',inspection,re.M|re.I);need(modules==['KERNEL32.dll'],'system-only direct imports '+tag)
  exes[tag]=dict(path=str(exe),sha256=sha(exe),machine=hex(machine),imports=modules)
 def execute(tag,args,label=None):
  p=subprocess.run([exes[tag]['path']]+list(map(str,args)),cwd=run,capture_output=True,text=True,timeout=120);(run/((label or tag)+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,tag+' execution: '+p.stderr);return p.stdout.strip()
 safety=execute('owned',[],'batch-safety');counts=re.search(r'checks=(\d+) actual_allocation_failures=(\d+)',safety);need(counts and int(counts[2])>100,'batch safety/allocation sweep')
 regressions={tag:execute(tag,['--test'] if tag in ['store','parameters'] else []) for tag in ['single','store','parameters','cached']}
 need('checks=888 actual_allocation_failures=788' in regressions['single'] and 'checks=4657' in regressions['store'] and 'checks=281' in regressions['parameters'] and 'checks=20 plus_prior=49' in regressions['cached'],'preceding safety counts')
 cases=corpus();nc=[native_case(q) for q in cases];predicted,event_counts,_=expected(nc);native_query=run/'native-query.bin';native_query.write_bytes(encode(nc));query=run/'owned-query.bin';query.write_bytes(struct.pack('<I',len(cases))+b''.join(struct.pack('<I',len(q))+b''.join(struct.pack('<II',*e) for e in q) for q in cases))
 execute('reference',[IFM,native_query,run/'native.bin']);need((run/'native.bin').read_bytes()==predicted,'entire original loop/capture/apply trace and independent model')
 projected=project_native((run/'native.bin').read_bytes(),nc);analytical=independent(cases);need(projected==analytical,'native integer phase/backup projection vs independent sequential model')
 execute('owned',['--wire',query,run/'owned.bin'],'owned-wire');need((run/'owned.bin').read_bytes()==projected,'actual owned stores vs native ordering/backups/phases')
 (run/'native-projected.bin').write_bytes(projected);(run/'independent.bin').write_bytes(analytical)
 benchmark=execute('benchmark',[BENCH,run/'benchmark.tsv']);old=json.loads(PRIOR.read_text());materials={};owners={};batches=[];row_counts=[0,0,0]
 for line in (run/'benchmark.tsv').read_text().splitlines():
  v=line.split('\t')
  if v[0]=='M':materials[v[1]]={'name':v[2],'key':v[3]}
  elif v[0]=='O':
   owners.setdefault(v[1],[]).append({'position':int(v[2]),'material':int(v[3])});need(materials.get(v[3],old['materials'][v[3]])['key']==v[4],'mapped resource key');row_counts=[a+int(b) for a,b in zip(row_counts,v[5:8])]
  elif v[0]=='B':
   batches.append(list(map(int,v[1:])));owners.setdefault(v[1],[])
 # The preceding audit emitted owners only for actual positions, omitting three
 # explicitly empty arrays. Compare that same domain; verify empty arrays below.
 need(materials==old['materials'] and {k:v for k,v in owners.items() if v}==old['owners'],'all benchmark material identities and nonempty owner positions equal accepted audit')
 need(len(batches)==107 and sum(b[1] for b in batches)==211 and sum(b[1]>0 for b in batches)==104,'benchmark arrays/positions')
 need(all((b[2:]==[0,0] if b[1] else b[2:]==[1,1]) for b in batches),'every unresolved batch refused; only empty lists observed')
 need(all(sha(k)==v for k,v in sources.items()) and all(sha(k)==v for k,v in protected.items()),'complete source/unrelated/input closure unchanged')
 need(sha(pre['editor'])==pre['editor_sha256'] and all(sha(ROOT/k)==v for k,v in pre['editor_dependencies'].items()),'editor and313 dependencies unchanged')
 result=dict(status='VERIFIED owned batch ordering/failure rollback and read-only benchmark mapping; application INCOMPLETE',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',batch_safety_checks=int(counts[1]),actual_allocation_failures=int(counts[2]),wire_cases=len(cases),source_positions=sum(map(len,cases)),native_trace_bytes=len(predicted),native_events=sum(event_counts.values()),projected_bytes=len(projected),projected_sha256=sha(run/'native-projected.bin'),alias_witness=dict(initial=7,applied=22,backups=[7,11],restored=11),safety=safety,regressions=regressions,benchmark=benchmark,benchmark_counts=dict(materials=len(materials),arrays=len(batches),positions=211,unresolved_refused=104,empty_observed=3,numeric_rows=row_counts[0],texture_rows=row_counts[1],ignored_rows=row_counts[2]),benchmark_source=str(BENCH),benchmark_sha256=sha(BENCH),prior_benchmark_audit=str(PRIOR),prior_benchmark_audit_sha256=sha(PRIOR),source=sources,protected=protected,parent=pre['parent'],parent_sha256=pre['parent_sha256'],executables=exes,editor=pre['editor'],editor_sha256=pre['editor_sha256'],editor_dependencies=pre['editor_dependencies'],editor_rebuilt=False,limits=['Original collection/capture/apply execute unchanged RX with controlled host variable/resource services; comparison is integer values/backups/order, not real native CMaterialVar lifetime or native failure rollback','Actual owned store publishes all unique-ID parameter views before observation; failure restores original pointers/forward values for all group targets','Success follows ordered typed backup replay, not universal initial-state restoration; source positions and repeated IDs retained, only ownership snapshots deduplicated','Resource IDs must already be resolved/retained; nullopt unavailable binding refuses whole group while engaged zero explicitly skips a source position','Benchmark proves identity/order mapping and unavailable-binding refusal only. No dummy material states, native loaded chain, active-shot selection, GPU or benchmark render compatibility claim','Independent per-position typed row backups; arbitrary shared native backup-element aliases/full mode2 Graph cloning not supported by this API','Owner-thread borrowing; caller keeps store alive. External explicit callback edits are detected but not undone; disjoint nested groups are independent','Texture handle ownership remains unavailable and type5 writes refuse; proxy evaluation not performed inside scope','No original writes/live SFM/computer control/GPU/editor launch/dependency install/publication'])
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','batch_safety_checks','actual_allocation_failures','wire_cases','native_events','projected_bytes','benchmark_counts']},indent=2),flush=True)
if __name__=='__main__':main()
