"""Measure original SetTexture/Clear reference order, without editor admission."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,re,struct,subprocess,tempfile,time
from verify_ifm_draw_packet import check_ranges
from verify_builtin_error_material_seed import source_closure
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/material-texture-value-lifetime-20261027'
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
NEW=['probe_material_texture_value_lifetime.cpp','build_material_texture_value_lifetime.cmd','verify_material_texture_value_lifetime.py']
ENV=0xffffffff
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b,s):
 if not b:raise RuntimeError(s)
def corpus():
 rows=[]
 sequences=[[1],[2],[ENV],[1,1,2,2,ENV,ENV,1],[ENV,1,ENV],[2,1,2],[1,2,1],[ENV,ENV]]
 for t in [0,1,2,3,4,6,7,8]:
  for old in ([1,2,ENV] if t==3 else [0,3] if t==8 else [0]):
   for metadata in [0,0x10,0x30,0x70,0x80,0xf0]:
    for remove in [0,1]:
     for components in ([1,2,3,4] if t==2 else [4]):
      for sequence in sequences:rows.append(dict(type=t,old=old,metadata=metadata,remove=remove,components=components,sequence=sequence))
 return rows
def expected(rows):
 lines=[];allocations=0;releases=0;counts=dict(retains=0,texture_releases=0,material_releases=0,delete_requests=0,identity_noops=0,environment_sets=0)
 for ci,c in enumerate(rows):
  t=c['type'];current=c['old'];refs=[1,1,1];allocation=int(t in [1,7]);free=0
  if t in [3,8] and current not in [0,ENV]:refs[current-1]+=1
  def drop(events):
   nonlocal free
   if t in [1,7]:events.append('F');free+=1
   elif t==3 and current!=ENV:
    refs[current-1]-=1;events.append('R'+str(current));counts['texture_releases']+=1
    if c['remove']:events.append('D'+str(current));counts['delete_requests']+=1
   elif t==8 and current:
    refs[current-1]-=1;events.append('M'+str(current));counts['material_releases']+=1
  for step,new in enumerate(c['sequence']+[0]):
   events=['Q']
   if new:
    if t==3 and current==new:counts['identity_noops']+=1
    else:
     if new!=ENV:refs[new-1]+=1;events.append('A'+str(new));counts['retains']+=1
     else:counts['environment_sets']+=1
     drop(events);t=3;current=new
   else:
    if t!=6:drop(events);t=6
   lines.append('\t'.join(map(str,['STEP',ci,step,new,c['metadata']|t,0,0,0,0,0,current,*refs,allocation,free,' '.join(events)+' ']))+'\n')
  need(refs==[1,1,1] and allocation==free,'independent final ownership balance');allocations+=allocation;releases+=free
 return ''.join(lines).encode(),allocations,releases,counts
def main():
 pre=json.loads((OUT/'preflight.json').read_text());need(all(sha(p)==h for p,h in pre['protected'].items()),'protected inputs before')
 need(sha(pre['parent'])==pre['parent_sha256']=='282a5c2bd87a2ca03c112ff43d60bbb739f79fadb11351b057e30856d1b52e94','accepted benchmark audit receipt');parent=json.loads(Path(pre['parent']).read_text())
 lib=Path(pre['library']);need(sha(lib)==pre['library_sha256']=='c78e414891b4d87e3922610a8bf937bca3efbaab95bced488522045998bc87f8','pinned private material system')
 image,evidence=check_ranges(lib,'materialsystem-disassembly.txt',[('whole_SetTexture',0x10019710,0x10019825),('release_previous_storage',0x10017ad0,0x10017b46),('whole_Clear',0x1001a0a0,0x1001a168)])
 need(image.word(0x10098f34+14*4)==0x10019710 and image.word(0x10098f34+18*4)==0x1001a0a0,'actual material variable vtable slots')
 targets=[image.word(0x10017b48+4*i) for i in range(8)];need(targets==[0x10017ae7,0x10017b44,0x10017af5,0x10017b44,0x10017b38,0x10017b44,0x10017b2a,0x10017b19],'independent storage-release jump table')
 sources=source_closure([Path(p) for p in parent['source']]+[ROOT/'work'/p for p in NEW]);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 (run/'static-reference.json').write_text(json.dumps(dict(ranges=evidence,release_dispatch=list(map(hex,targets))),indent=2)+'\n')
 started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_texture_value_lifetime.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180);(run/'build-driver.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'fresh reference build')
 exe=OUT/'texture_value_reference_x86.exe';b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',b,pe+4)[0]==0x14c and struct.unpack_from('<H',b,pe+24)[0]==0x10b and b'asInvoker' in b,'fresh x86 reference PE32 asInvoker')
 log=(OUT/'reference-build.log').read_text();(run/'reference-build.log').write_text(log);need(not re.search(r'\b(?:warning|error) C\d+',log),'clean reference compile')
 inspect=subprocess.run([str(DUMP),'/headers','/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/'inspection.txt').write_text(inspect);imports=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',inspect,re.I|re.M);need(imports==['KERNEL32.dll'],'only Windows direct imports')
 cases=corpus();query=run/'queries.tsv';query.write_text(''.join(' '.join(map(str,[ci,c['type'],c['old'],c['metadata'],c['remove'],c['components'],*c['sequence']]))+'\n' for ci,c in enumerate(cases)))
 want,allocated,freed,counts=expected(cases);(run/'expected.tsv').write_bytes(want);trace=run/'reference.tsv'
 p=subprocess.run([str(exe),str(lib),str(query),str(trace)],cwd=run,capture_output=True,text=True,timeout=120);(run/'reference-run.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'original setter execution: '+p.stdout+p.stderr)
 actual=trace.read_bytes()
 if actual!=want:
  a=actual.decode().splitlines();w=want.decode().splitlines();at=next((i for i,(x,y) in enumerate(zip(a,w)) if x!=y),min(len(a),len(w)));raise RuntimeError('independent transition mismatch at row '+str(at)+' actual='+str(a[at:at+1])+' expected='+str(w[at:at+1]))
 transitions=want.count(b'\n');m=re.search(r'cases=(\d+) transitions=(\d+) allocations=(\d+) releases=(\d+)',p.stdout);need(m and list(map(int,m.groups()))==[len(cases),transitions,allocated,freed],'complete case and lifetime counts')
 regressions={}
 for name,entry in parent['reused_executables'].items():
  need(sha(entry['path'])==entry['sha256'],'accepted x64 '+name+' identity');test=subprocess.run([entry['path']],cwd=run,capture_output=True,text=True,timeout=120);(run/(name+'-regression.log')).write_text(test.stdout+test.stderr);need(test.returncode==0 and test.stdout==parent['regressions'][name],'unchanged x64 '+name+' safety');regressions[name]=test.stdout
 need(all(sha(p)==h for p,h in sources.items()) and all(sha(p)==h for p,h in pre['protected'].items()),'all existing sources/protected inputs unchanged after')
 need(sha(pre['editor'])==pre['editor_sha256'] and all(sha(ROOT/p)==h for p,h in pre['editor_dependencies'].items()),'editor and313 dependencies unchanged')
 limits=['Independent native x64 application remains INCOMPLETE. This new executable is explicitly an isolated x86 reference investigation, not a new x64 runtime component or texture implementation',
 'Original whole SetTexture slot14 and Clear slot18 execute with unchanged RX code and no DllMain/import resolution/SFM process. Private known material-system copy only. Current installation ifm.dll/gameinfo.txt intentional changes are not overwritten or treated as corruption',
 'Resource retain/release/delete services use controlled external texture/material spies and a baseline external reference; old resource type3/8 seeded in constructor-initialized variables, not full engine texture/material construction. Constructor-backed numeric, string, vector, undefined and matrix states also exercised',
 'Queue service always returns null, parent material is absent. No queue redirection, parent notifications, live renderer effects, concurrency, exceptions from native callbacks or resource reclamation implementation claim',
 'Environment token0xffffffff is distinguished from a texture. Native setter statically dereferences ordinary null texture on a change; unsafe null call was deliberately NOT executed. No owned null policy or fake resource is introduced. FourCC/unknown storage is outside measured transition corpus',
 'No owned candidate/header, store integration, resource cache, FindTexture request/missing/error policy, VTF decoding or GPU implementation changed. Existing type5 override refusal remains. Prior matching controlled IFM dispatch does not prove joint real IFM+material execution',
 'Complete editor not rebuilt because all313 dependencies and executable hash remain unchanged. Existing x64 profile and binding safety executables rerun by accepted hash, not freshly compiled. No original/session writes, computer use, screen capture, window/process enumeration or publication']
 result=dict(status='VERIFIED bounded original texture setter/reference lifetime; owned texture support INCOMPLETE',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',cases=len(cases),transitions=transitions,exact_trace_bytes=len(want),allocations=allocated,releases=freed,counts=counts,native_log=p.stdout,executable=dict(path=str(exe),sha256=sha(exe),machine='0x14c',imports=imports,fresh=True),regressions=regressions,reused_executables=parent['reused_executables'],source=sources,protected=pre['protected'],parent=pre['parent'],parent_sha256=pre['parent_sha256'],read_inputs={str(lib):sha(lib),str(ROOT/'work/materialsystem-disassembly.txt'):sha(ROOT/'work/materialsystem-disassembly.txt')},editor=pre['editor'],editor_sha256=pre['editor_sha256'],editor_dependencies=pre['editor_dependencies'],editor_rebuilt=False,query_sha256=sha(query),trace_sha256=sha(trace),static_sha256=sha(run/'static-reference.json'),limits=limits)
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','transitions','exact_trace_bytes','allocations','releases','counts','regressions']},indent=2),flush=True)
if __name__=='__main__':main()
