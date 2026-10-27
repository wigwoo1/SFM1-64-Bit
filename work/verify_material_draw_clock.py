"""Whole original dispatcher/clock methods; draw chain is static evidence only.

Does not start SFM, load original imports or change owned renderer admission.
"""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,random,re,struct,subprocess,tempfile,time
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/material-draw-clock-order-20261027'
GAME=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
MATERIAL=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/materialsystem.dll'
IFM=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/tools/ifm.dll'
CLIENT=ROOT/'outputs/material-proxy-evaluation-20261027/copied-bin/client.dll'
ENGINE=GAME/'bin/engine.dll';STUDIO=GAME/'bin/studiorender.dll'
SESSION=GAME/'tf_movies/elements/sessions/mtt_soldier/mtt_soldier.dmx'
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
NEW=['probe_material_draw_clock.cpp','build_material_draw_clock.cmd','verify_material_draw_clock.py']
def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def need(b,s):
 if not b:raise RuntimeError(s)
def pack(words):return struct.pack('<'+'I'*len(words),*words)
def dispatch_cases():
 out=[]
 for n in [0,1,2,7,8,255]:
  for mode in [0,1,4,255]:
   for cut,new in [(0,n)]+([(1,0),(1,n),(n,0),(max(1,n//2),max(0,n-1))] if n else []):
    for order in [list(range(n)),list(reversed(range(n))),[0]*n]:out.append((mode,n,cut,new,0xfffffff0,order))
 rng=random.Random(2026102771)
 for _ in range(512):
  n=rng.randrange(256);out.append((rng.choice([0,0,0,1,4,255]),n,rng.randrange(n+1),rng.randrange(n+1),rng.getrandbits(32),[rng.randrange(n) for _ in range(n)]))
 return out
def dispatch_prediction(cases):
 out=bytearray(pack([len(cases)]));events=0;mutated=0
 for ci,(mode,n,cut,new,value,order) in enumerate(cases):
  live=n;trace=[];i=0
  while mode==0 and i<live:
   identity=order[i];before=value;value=(value+identity+1)&0xffffffff;trace.extend([identity,before,value]);i+=1
   if cut and i==cut:live=new;mutated+=1
  events+=i;out+=pack([ci,1,live,value,i]+trace)
 return bytes(out),events,mutated
def clock_cases():
 special=[0,0x80000000,1,0x80000001,0x007fffff,0x00800000,0x3f800000,0x40a00000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc00000,0xffc12345,0x7f800001,0xff800001,0x7fbfffff]
 out=[(0x40a00000,0x3c23d70a,0x3f000000)]
 out.extend((a,b,c) for i,a in enumerate(special) for b in special for c in [special[(i+3)%len(special)]])
 rng=random.Random(2026102772);out.extend(tuple(rng.getrandbits(32) for _ in range(3)) for _ in range(512));return out
def quiet(bits):return bits|0x400000 if bits&0x7f800000==0x7f800000 and bits&0x7fffff else bits
def main():
 pre=json.loads((OUT/'preflight.json').read_text());parent=json.loads(Path(pre['parent']).read_text())
 need(sha(pre['parent'])==pre['parent_sha256']=='bc2f7db24fc315037416f51b55fc80f707047b8304f9fd4a83d4424774f7e890','accepted parent')
 need(all(sha(k)==v for k,v in pre['protected'].items()),'all protected source/inputs before')
 pinned={MATERIAL:'c78e414891b4d87e3922610a8bf937bca3efbaab95bced488522045998bc87f8',IFM:'8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4',CLIENT:'91dd1062227b095f1338312aa7268121894970c3e0a5b0d314e42c77f2aba73b',ENGINE:'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f',STUDIO:'dc7eef7620f2a687c3cdd4182f2f17f0d78890e2748abcbe923b459b5fa340f7'}
 need(all(sha(k)==v for k,v in pinned.items()),'original input identities')
 sources=source_closure([Path(k) for k in parent['source']]+[ROOT/'work'/k for k in NEW])
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 material,mr=check_ranges(MATERIAL,'materialsystem-disassembly.txt',[('whole_normal_and_diagnostic_dispatch',0x10004980,0x10004aba),('forwarded_slot53',0x10007700,0x10007711)])
 engine,er=check_ranges(ENGINE,'engine-disassembly.txt',[('tool_get_current',0x101e5780,0x101e5787),('tool_get_frame',0x101e5790,0x101e5797),('tool_set_frame_NOT_current',0x101e57b0,0x101e57c4),('client_init_globals_argument',0x10060cfb,0x10060d16),('model_registration',0x10308e90,0x10308ea5),('model_vtable_constructor',0x100d8d80,0x100d8d9c),('model_slot16_static',0x100d69f0,0x100d6ae6),('model_slot18_static',0x100d67d0,0x100d683f),('model_slot19_static',0x100dabe0,0x100dafc4)])
 client,cr=check_ranges(CLIENT,'../outputs/material-proxy-evaluation-20261027/client-disassembly-private.txt',[('client_stores_globals_argument',0x10099896,0x100998a4),('sine_current_time_load',0x101364f0,0x101365a7),('scroll_current_time_load',0x10198b80,0x10198d40)])
 ifm,ir=check_ranges(IFM,'ifm-disassembly.txt',[('override_capture_apply_call',0x102438a6,0x102438c2),('normal_draw_packet_and_dispatch',0x10243a57,0x10243b8c),('backup_replay_call',0x10243fca,0x10243fe3),('model_interface_acquisition',0x102baff0,0x102bb00e)])
 _,sr=check_ranges(STUDIO,'studiorender-disassembly.txt',[('material_proxy_bind_helper_static',0x10053cd0,0x10053e5f),('one_helper_call_static',0x1005748d,0x100574af)])
 need(material.word(0x10095d74+53*4)==0x10004980 and material.word(0x10095a9c+53*4)==0x10007700,'actual/forwarded material slot53')
 need([engine.word(0x103666c4+i*4) for i in [47,48,50]]==[0x101e5780,0x101e5790,0x101e57b0],'EngineTool clock vtable')
 need([engine.word(0x1034086c+i*4) for i in [16,18,19]]==[0x100d69f0,0x100d67d0,0x100dabe0],'VEngineModel draw vtable')
 need(ifm.string(0x10ae74a8)==engine.string(0x10340d78)=='VEngineModel016','draw interface is VEngineModel, not EngineTool')
 # Require the exact current-time pointer and +0C load in both native proxies.
 for ranges in [[cr[1]],[cr[2]]]:
  rows=[x['instruction'] for q in ranges for x in q['instructions']]
  need(any('1063CE48h' in x for x in rows) and any('+0Ch]' in x for x in rows),'proxy uses client globals current time field')
 (run/'static-reference.json').write_text(json.dumps(dict(material=mr,engine=er,client=cr,ifm=ir,studio=sr),indent=2))
 started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_draw_clock.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180);(run/'build-driver.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'fresh two original-method probe builds')
 exes={}
 for tag in ['dispatch','clock']:
  exe=OUT/(tag+'_reference_x86.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];need(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==0x14c and struct.unpack_from('<H',raw,pe+24)[0]==0x10b and b'asInvoker' in raw,'fresh PE32/asInvoker '+tag)
  log=(OUT/(tag+'-build.log')).read_text();(run/(tag+'-build.log')).write_text(log);need(not re.search(r'\b(?:warning|error)\b',log,re.I),'clean compile '+tag)
  inspection=subprocess.run([str(DUMP),'/headers','/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(tag+'-inspection.txt')).write_text(inspection);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',inspection,re.M|re.I);need(modules==['KERNEL32.dll'],'system-only direct imports '+tag);exes[tag]=dict(path=str(exe),sha256=sha(exe),machine='0x14c',imports=modules,fresh=True)
 def execute(exe,args,label):
  p=subprocess.run([str(exe)]+list(map(str,args)),cwd=run,capture_output=True,text=True,timeout=180);(run/(label+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,label+' failed: '+p.stdout+p.stderr);return p.stdout.strip()
 dispatch=dispatch_cases();query=run/'dispatch-query.bin';query.write_bytes(pack([len(dispatch)])+b''.join(pack(list(q[:5])+q[5]) for q in dispatch));predicted,events,mutations=dispatch_prediction(dispatch)
 dispatch_log=execute(exes['dispatch']['path'],[MATERIAL,query,run/'dispatch.bin'],'dispatch');need((run/'dispatch.bin').read_bytes()==predicted,'entire original dispatcher output vs independent order/live-count model');(run/'dispatch-expected.bin').write_bytes(predicted)
 clocks=clock_cases();cq=run/'clock-query.bin';cq.write_bytes(pack([len(clocks)])+b''.join(pack(q) for q in clocks));cp=pack([len(clocks)])+b''.join(pack([i,current,replacement,quiet(current),quiet(replacement)]) for i,(current,old,replacement) in enumerate(clocks))
 clock_log=execute(exes['clock']['path'],[ENGINE,cq,run/'clock.bin'],'clock');need((run/'clock.bin').read_bytes()==cp,'entire original frame setter/current/frame getter output and independent raw-bit prediction');(run/'clock-expected.bin').write_bytes(cp)
 # Reuse only the exact accepted owned x64 runtime: no owned source changed.
 pilot=parent['executables']['pilot'];need(sha(pilot['path'])==pilot['sha256']=='b77d14d7f508c42deab4df9a4b58a673169f9b7325ccca6779563bb1021fa0fd','previously tested owned runtime executable identity')
 pilot_log=execute(pilot['path'],[SESSION,GAME,run/'pilot.tsv'],'pilot-reused-x64');need('checks=132' in pilot_log and sha(run/'pilot.tsv')==parent['frame_evidence_sha256'],'owned actual-file seven-time runtime evidence unchanged')
 need(all(sha(k)==v for k,v in sources.items()) and all(sha(k)==v for k,v in pre['protected'].items()) and all(sha(k)==v for k,v in pinned.items()),'complete source/original/protected closure unchanged')
 need(sha(pre['editor'])==pre['editor_sha256'] and all(sha(ROOT/k)==v for k,v in pre['editor_dependencies'].items()),'editor and313 dependencies unchanged')
 limits=['Original whole unchanged CMaterial slot53 executes only tracking-disabled normal mode0 and skip modes1/4/255, with host proxy callbacks and opaque bind data, not native built-in proxies or GPU','Controlled count edits establish live byte-count rereading; they do not authorize concurrent/reentrant mutation in owned store','Tracking-enabled MaterialVar access wrappers and diagnostic modes2/3 not executed','Whole original EngineTool setter/getters execute; current-time vs frame-interval data mapping also verified by initialization/client load bytes','IFM override/draw/replay ordering, VEngineModel registration/vtables, and StudioRender material slot53 helper are STATIC byte-checked evidence, not one dynamically composed native draw','Middle engine-to-StudioRender eligibility/queue/actual helper selection is not fully traced here; no universal branch or proxy-per-frame count claim','Session sample/shot/frame/subsample to original engine current-time mapping remains UNVERIFIED; slot50 must not be used as current-time setter','Owned session batch still forbids proxy evaluation inside pending scope; do not simply remove guard because successful backup replay must preserve proxy changes outside overridden parameters','Owned actual pilot x64 executable was reused by exact accepted SHA and rerun on new evidence output; no fresh owned/editor compile needed because their dependency closures unchanged','No original writes, SFM/process/window interaction, GPU rendering, serialization/document/history changes, new dependency, publication or renderer admission']
 result=dict(status='VERIFIED bounded native dispatcher and clock field behavior; composed draw/clock integration PARTIALLY VERIFIED',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',dispatch_cases=len(dispatch),dispatch_events=events,controlled_count_mutations=mutations,dispatch_bytes=len(predicted),clock_cases=len(clocks),clock_bytes=len(cp),clock_witness=dict(current_time_before='0x40a00000 (5 seconds)',frame_interval_before='0x3c23d70a',slot50_argument='0x3f000000 (.5 seconds)',current_time_after='0x40a00000',frame_interval_after='0x3f000000'),dispatch_log=dispatch_log,clock_log=clock_log,pilot_log=pilot_log,executables=exes,reused_owned_pilot=pilot,source=sources,protected=pre['protected'],read_inputs={str(k):v for k,v in pinned.items()},parent=pre['parent'],parent_sha256=pre['parent_sha256'],editor=pre['editor'],editor_sha256=pre['editor_sha256'],editor_dependencies=pre['editor_dependencies'],editor_rebuilt=False,static_reference_sha256=sha(run/'static-reference.json'),dispatch_evidence_sha256=sha(run/'dispatch.bin'),clock_evidence_sha256=sha(run/'clock.bin'),pilot_evidence_sha256=sha(run/'pilot.tsv'),limits=limits)
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','dispatch_cases','dispatch_events','controlled_count_mutations','clock_cases','pilot_log']},indent=2),flush=True)
if __name__=='__main__':main()
