"""Composed owned CPU draws; native replay witness has controlled setter writes."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,random,re,struct,subprocess,tempfile,time
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges
ROOT=Path(__file__).resolve().parent.parent;OUT=ROOT/'outputs/material-session-draw-transaction-20261027'
GAME=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game');SESSION=GAME/'tf_movies/elements/sessions/mtt_soldier/mtt_soldier.dmx'
IFM=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/tools/ifm.dll'
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
NEW=['test_material_session_draw_transaction.cpp','probe_material_session_draw_replay.cpp','build_material_session_draw_transaction.cmd','verify_material_session_draw_transaction.py']
def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def need(b,s):
 if not b:raise RuntimeError(s)
def pack(q):return struct.pack('<'+'I'*len(q),*q)
def corpus():
 cases=[]
 for n in [0,1,2,7,32]:
  for binds in [0,1,2,8]:
   for a,b,c in [(7,9,99),(0xffffffff,0x80000000,0),(0,0xffffffff,0x7fffffff)]:cases.append((n,binds,a,b,c,[11+i for i in range(n)]))
 rng=random.Random(2026102774)
 for _ in range(512):
  n=rng.randrange(33);cases.append((n,rng.randrange(9),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),[rng.getrandbits(32) for _ in range(n)]))
 return cases
def predicted(cases):
 out=bytearray(pack([len(cases)]))
 for ci,(n,binds,a,b,c,rows) in enumerate(cases):
  backups=[]
  for v in rows:backups.append(a);a=v
  out+=pack([ci,n,binds])
  for _ in range(binds):b=a;a=c;out+=pack([a,b])
  out+=pack(backups)
  for v in backups:a=v
  out+=pack([a,b])
 return bytes(out)
def main():
 pre=json.loads((OUT/'preflight.json').read_text());parent=json.loads(Path(pre['parent']).read_text());need(sha(pre['parent'])==pre['parent_sha256']=='c20e68de47f58101d1a00bd25297d380926e46e59bf262d8f4e8d6f07d5ef623','accepted parent')
 protected={p:h for p,h in pre['protected'].items() if p not in pre['intended_changes']};need(all(sha(p)==h for p,h in protected.items()),'preceding unrelated source/inputs before')
 need(sha(OUT/'before/material_object_store_candidate.h')==parent['source'][str(ROOT/'work/material_object_store_candidate.h')],'saved old store')
 pilotReceipt=ROOT/'outputs/material-pilot-runtime-20261027/verification-0dp9umdi/results.json';need(sha(pilotReceipt)=='bc2f7db24fc315037416f51b55fc80f707047b8304f9fd4a83d4424774f7e890','accepted real pilot receipt');pilot=json.loads(pilotReceipt.read_text());reads=pilot['read_inputs'];need(all(sha(p)==h for p,h in reads.items()),'read-only pilot inputs')
 need(sha(IFM)=='8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4' and sha(SESSION)=='5a08cf8eeb086586f05dc856fb5469dd238fde90315ecf17c2dd4ab5f0742aee','original IFM/session')
 sources=source_closure([Path(p) for p in parent['source']]+[ROOT/'work'/n for n in NEW]+[ROOT/'work/probe_material_session_collection.cpp'])
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 _,ranges=check_ranges(IFM,'ifm-disassembly.txt',[('whole_collection',0x1023e3e0,0x1023e4b4),('whole_capture',0x1023c0a0,0x1023c286),('whole_apply',0x1023e140,0x1023e3a4),('resource_get',0x105c1220,0x105c1260)]);(run/'static-reference.json').write_text(json.dumps(ranges,indent=2))
 started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_session_draw_transaction.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=300);(run/'build-driver.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'fresh nine executable builds')
 exes={}
 for tag,name,machine in [('reference','draw_replay_reference_x86',0x14c),('draw','session_draw_owned_x64',0x8664),('batch','batch_regression_x64',0x8664),('single','single_regression_x64',0x8664),('bindings','bindings_regression_x64',0x8664),('pilot','pilot_regression_x64',0x8664),('store','store_regression_x64',0x8664),('parameters','parameters_regression_x64',0x8664),('cached','cached_regression_x64',0x8664)]:
  exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];need(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'fresh architecture/manifest '+tag)
  log=(OUT/(tag+'-build.log')).read_text();(run/(tag+'-build.log')).write_text(log);need(not re.search(r'\b(?:warning|error)\b',log,re.I),'clean compile '+tag)
  inspect=subprocess.run([str(DUMP),'/headers','/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(tag+'-inspection.txt')).write_text(inspect);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',inspect,re.I|re.M);need(modules==['KERNEL32.dll'],'system-only imports '+tag);exes[tag]=dict(path=str(exe),sha256=sha(exe),machine=hex(machine),imports=modules)
 def execute(tag,args,label=None):
  p=subprocess.run([exes[tag]['path']]+list(map(str,args)),cwd=run,capture_output=True,text=True,timeout=180);(run/((label or tag)+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,tag+' failed: '+p.stdout+p.stderr);return p.stdout.strip()
 safety=execute('draw',[],'draw-safety');m=re.search(r'checks=(\d+) actual_allocation_failures=(\d+)',safety);need(m and int(m[2])>100,'actual composed allocation failure coverage')
 regressions={tag:execute(tag,['--test'] if tag in ['store','parameters'] else []) for tag in ['batch','single','bindings','store','parameters','cached']}
 for tag,needle in [('batch','checks=1577 actual_allocation_failures=1519'),('single','checks=888 actual_allocation_failures=788'),('bindings','checks=606 allocation_failures=572'),('store','checks=4657'),('parameters','checks=281'),('cached','checks=20 plus_prior=49')]:need(needle in regressions[tag],'unchanged legacy safety coverage '+tag)
 cases=corpus();query=run/'queries.bin';query.write_bytes(pack([len(cases)])+b''.join(pack(list(q[:5])+q[5]) for q in cases));want=predicted(cases);(run/'expected.bin').write_bytes(want)
 native=execute('reference',[IFM,query,run/'native.bin']);need((run/'native.bin').read_bytes()==want,'whole native collection/capture/replay with CONTROLLED interposed writes vs independent predictor')
 owned=execute('draw',['--wire',query,run/'owned.bin'],'draw-wire');need((run/'owned.bin').read_bytes()==want,'actual owned Equals pair/snapshots/replay vs native selected projection')
 pilotDraw=execute('draw',['--pilot',SESSION,GAME,run/'pilot-draw.tsv'],'actual-pilot-draw');need('actual_pilot_draw_binds=2 proxy_instances=7 journal_writes=12' in pilotDraw,'actual file composed bind coverage')
 expectedFrames={int(v[1]):v[1:] for line in (pilotReceipt.parent/'pilot.tsv').read_text().splitlines() if (v:=line.split('\t'))[0]=='T'}
 rows=[line.split('\t') for line in (run/'pilot-draw.tsv').read_text().splitlines()];draws=[v for v in rows if v[0]=='D'];need(len(draws)==2 and all(v[1:]==expectedFrames[int(v[1])] for v in draws),'full actual pilot per-bind numerical evidence equals accepted component/runtime trace at explicit .125 and5 seconds');need(len([v for v in rows if v[0]=='R'])==1,'pilot replay assertion emitted')
 pilotRegression=execute('pilot',[SESSION,GAME,run/'pilot-regression.tsv'],'pilot-regression');need('checks=132' in pilotRegression and sha(run/'pilot-regression.tsv')==pilot['frame_evidence_sha256'],'all132 preceding pilot tests and complete TSV unchanged')
 need(all(sha(p)==h for p,h in sources.items()) and all(sha(p)==h for p,h in protected.items()) and all(sha(p)==h for p,h in reads.items()),'complete current source/unrelated/original read audit')
 need(sha(pre['editor'])==pre['editor_sha256'] and all(sha(ROOT/p)==h for p,h in pre['editor_dependencies'].items()),'editor313 dependencies unchanged')
 limits=['New owned explicit CPU draw sequence is opt-in; no native clock mapping, automatic selection/queue scheduling, texture ownership, GPU/editor admission or full renderer parity','Original whole IFM collection/capture/apply/replay executes unchanged RX with controlled host materials/variables. Interposed integer setter writes are authored fixture actions, NOT original built-in proxy execution; comparison selects integer snapshots/backups/final states, not native full MaterialSystem state','Owned wire invokes actual existing Equals instances via staged frame inputs. Actual pilot uses all7 real-file proxies and checked real session model context with authored one-row override and explicit times, not actual benchmark full overrides/shot mapping or exact native full composition','Caller supplies retained actual IDs and explicit per-bind inputs; repeated IDs evaluate repeatedly, draw-only IDs included. Missing time/context required by a proxy refuses group; unknown is not zero','Observer borrows immutable per-bind parameter snapshots/journals and final actual store publications for call only. Per-bind snapshots do not retain texture/GPU resources or authorize asynchronous use after callback','Successful replay affects only captured typed rows, preserving other proxy writes. Owned failure restores original parameter pointers/forwarding, not arbitrary side effects inside third-party proxy/services, external loader/registry mutations, explicit root edits or GPU work','Legacy withSessionCollection/single-scope APIs and public evaluateProxies pending-scope refusal retained; original allocation failure counts unchanged','No original writes/live SFM/computer use, document ownership/history/serialization redesign, new dependency, publication or full editor rebuild']
 result=dict(status='VERIFIED bounded owned override/proxy/snapshot/replay; renderer and clock integration INCOMPLETE',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',safety=safety,safety_checks=int(m[1]),actual_allocation_failures=int(m[2]),wire_cases=len(cases),bind_snapshots=sum(q[1] for q in cases),override_positions=sum(q[0] for q in cases),projection_bytes=len(want),projection_sha256=sha(run/'native.bin'),native=native,owned=owned,pilot_draw=pilotDraw,pilot_regression=pilotRegression,regressions=regressions,executables=exes,source=sources,protected=protected,read_inputs=reads,parent=pre['parent'],parent_sha256=pre['parent_sha256'],editor=pre['editor'],editor_sha256=pre['editor_sha256'],editor_dependencies=pre['editor_dependencies'],editor_rebuilt=False,static_reference_sha256=sha(run/'static-reference.json'),pilot_draw_evidence_sha256=sha(run/'pilot-draw.tsv'),limits=limits)
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','safety_checks','actual_allocation_failures','wire_cases','bind_snapshots','projection_bytes','pilot_draw','regressions']},indent=2),flush=True)
if __name__=='__main__':main()
