"""Whole IFM material collection/capture/apply order, explicit host resources.

No original process, writes, loader, instruction patches or GPU. Verifies shared
material/element identity, partial backups, lazy acquisition and typed actions;
does not execute real CMaterialVar getters/setters, shader init or DrawModel.
"""
import copy,hashlib,json,random,re,struct,subprocess,tempfile,time
from pathlib import Path
from datetime import datetime,timezone
from verify_ifm_draw_packet import check_ranges
from verify_builtin_error_material_seed import source_closure
from verify_material_session_backup import quiet,same_float,color
ROOT=Path(__file__).resolve().parent.parent;OUT=ROOT/'outputs/material-session-collection-20261027'
IFM=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/tools/ifm.dll'
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
NEW=['material_session_collection_candidate.h','probe_material_session_collection.cpp','build_material_session_collection.cmd','verify_material_session_collection.py','test_material_session_collection.cpp']
def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def need(b,s):
 if not b:raise RuntimeError(s)
def attr(kind=2,name='$a',words=None,text='texA',meta=0,state=0):return dict(flags=kind|meta,nameState=state,words=list(words) if words is not None else [11]*16,name=name,text=text)
def element(attrs=None,cached=1,lookup=1,available=1,kind=1,name_state=0):return dict(available=available,type=kind,cached=cached,lookup=lookup,nameState=name_state,attrs=copy.deepcopy(attrs) if attrs is not None else [attr()])
def case(label,elements=None,source=None,backup=None,flags=3,phase=0,empty=0,presence=3):
 mats=[dict(refs=0,vars=[dict(presence=presence,words=[7]*16,text='texA'),dict(presence=3,words=[0x3f000000]*16,text='texB')]),dict(refs=0,vars=[dict(presence=3,words=[9]*16,text='texB'),dict(presence=presence,words=[0x3f800000]*16,text='texA')])]
 return dict(label=label,materials=mats,elements=elements if elements is not None else [element(),element([attr(words=[0]*16)])],source=[1] if source is None else source,backup=[2] if backup is None else backup,flags=flags,phase=phase,emptyReturn=empty)
def corpus():
 out=[];rng=random.Random(2026102757)
 # Named witness: A and B share material1; backupB captures A's applied11.
 # RestoreA(original7) then RestoreB(11) leaves11, not initial7.
 out.append(case('shared-material-restore-is-not-universal-rollback',[element([attr(words=[11]*16)]),element([attr(words=[22]*16)]),element([attr(words=[0]*16)]),element([attr(words=[0]*16)])],[1,2],[3,4]))
 out.append(case('backup-resource-name-mismatch-restores-other-material',[element(),element([attr(words=[0]*16)],cached=2,lookup=2)]))
 out.append(case('same-element-in-source-and-backup',source=[1],backup=[1]))
 out.append(case('repeated-source-and-backup-handles',source=[1,1,1],backup=[2,2,2],flags=7))
 for source in [[],[0],[99],[1],[2],[1,2],[2,1],[1,1,2]]:
  for backup in [[],[0],[99],[2],[1],[2,2],[2,1,0]]:
   for flags in [0,1,3,7]:out.append(case('handle-index-order',source=source,backup=backup,flags=flags))
 for available in [0,1]:
  for material_type in [0,1]:
   for cached in [0,1,2]:
    for lookup in [0,1,2]:
     for ns in [0,1,2]:
      for empty in [0,2]:out.append(case('source-type-and-lazy-cache',[element(cached=cached,lookup=lookup,available=available,kind=material_type,name_state=ns),element(cached=0,lookup=2,name_state=ns)],empty=empty,flags=7))
 for available in [0,1]:
  for material_type in [0,1]:
   for name_state in [0,1,2]:out.append(case('backup-invalid-or-different-acquisition',[element(),element(cached=0,lookup=2,available=available,kind=material_type,name_state=name_state)],flags=7,empty=1))
 for kind in range(32):
  for old_kind in [2,3,4,5,8,9,10,11,14]:
   for phase in [0,3,4]:out.append(case('mixed-typed-source-backup',[element([attr(kind)]),element([attr(old_kind,words=[0]*16)])],phase=phase))
 for kind in [2,3,4,5,8,9,10,11,14]:
  for presence in range(4):
   for meta in [0,0x20,0x400,0x80,0x4080,0x2080,0xffff0000]:out.append(case('presence-flags-and-repeat',[element([attr(kind,meta=meta)]),element([attr(kind,words=[0]*16,meta=meta)])],presence=presence,flags=7))
 special=[0,0x80000000,1,0x80000001,0x3f000000,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc12345,0x7fa12345]
 for kind in [3,8,9,10,11,14]:
  for word in special:
   q=case('typed-special-values',[element([attr(kind,words=[word]*16)]),element([attr(kind,words=[word]*16)])],flags=7);q['materials'][0]['vars'][0]['words']=[word]*16;out.append(q)
 for kind in [5,2,14]:
  for name in ['$a','$b','$FLAGS','$flags2','$flags_defined','plain','$missing','$a\0suffix']:
   for state in [0,1,2]:out.append(case('names-and-chain-order',[element([attr(kind,name, state=state),attr(2,'$b'),attr(3,'$a')]),element([attr(kind,name,words=[0]*16,state=state),attr(2,'$a')])],flags=7))
 for _ in range(384):
  es=[]
  for i in range(rng.randrange(1,9)):
   attrs=[attr(rng.randrange(32),rng.choice(['$a','$b','$a','$flags','$missing']),[rng.getrandbits(32) for _ in range(16)],rng.choice(['','texA','texB','other']),rng.choice([0,0x20,0x400,0x4080]),rng.randrange(3)) for _ in range(rng.randrange(5))]
   es.append(element(attrs,cached=rng.randrange(3),lookup=rng.randrange(3),available=rng.randrange(2),kind=rng.randrange(2),name_state=rng.randrange(3)))
  out.append(case('seeded-alias-index-cache',es,[rng.randrange(len(es)+2) for _ in range(rng.randrange(8))],[rng.randrange(len(es)+2) for _ in range(rng.randrange(8))],rng.choice([0,1,3,7]),rng.choice([0,1,2,3,4,5]),rng.randrange(3),rng.randrange(4)))
 return out
def encode(cases):
 out=bytearray(struct.pack('<I',len(cases)))
 def text(s):
  b=s.encode();return struct.pack('<I',len(b))+b
 for q in cases:
  out+=struct.pack('<7I',len(q['materials']),len(q['elements']),len(q['source']),len(q['backup']),q['flags'],q['emptyReturn'],q['phase']&0xffffffff)
  for m in q['materials']:
   for v in m['vars']:out+=struct.pack('<17I',v['presence'],*v['words'])+text(v['text'])
  for e in q['elements']:
   out+=struct.pack('<6I',e['available'],e['type'],e['cached'],e['lookup'],e['nameState'],len(e['attrs']))
   for a in e['attrs']:out+=struct.pack('<18I',a['flags'],a['nameState'],*a['words'])+text(a['name'])+text(a['text'])
  out+=struct.pack('<'+'I'*(len(q['source'])+len(q['backup'])),*q['source'],*q['backup'])
 return out
def simulate(q):
 q=copy.deepcopy(q);ms=q['materials'];es=q['elements'];events=[];snapshots=[]
 for e in es:
  for a in e['attrs']:
   if a['flags']&31==5:a['words'][0]=0
 def event(kind,a,b=0,c=0,words=None):events.append((kind,a,b,c,*(list(words) if words is not None else []),*([0]*(16-(len(words) if words is not None else 0)))))
 def resolve(h):
  ok=h if 0<h<=len(es) and es[h-1]['available'] else 0;event(0,h,ok);return ok
 def ismat(h):event(1,h,es[h-1]['type']);return es[h-1]['type']!=0
 def acquire(h):
  e=es[h-1]
  if e['cached']:return e['cached']
  if e['nameState']==1:return 0
  result=q['emptyReturn'] if e['nameState']==2 else e['lookup'];event(2,0 if e['nameState']==2 else h,result);e['cached']=result
  if result:ms[result-1]['refs']+=1;event(15,result)
  return result
 def find(m,a):
  name=a['name'].split('\0')[0]
  if a['nameState'] or not name.startswith('$') or name.lower() in ['$flags','$flags2']:return None,None
  v={'$a':0,'$b':1}.get(name,0xffffffff);presence=ms[m-1]['vars'][v]['presence'] if v<2 else 0;event(3,m,v,presence)
  return (v,ms[m-1]['vars'][v]) if presence==3 else (None,None)
 def capture(h,m):
  if not h:return
  for a in es[h-1]['attrs']:
   vi,s=find(m,a)
   if s is None:continue
   t=a['flags']&31;v=a['words'].copy();text=a['text'];w=s['words'];different=False
   if t in [2,4]:event(4,m,vi);v[0]=w[0] if t==2 else (v[0]&0xffffff00)|int(w[0]!=0);different=v[0]!=a['words'][0]
   elif t==3:event(5,m,vi);v[0]=quiet(w[0]);different=not same_float(v[0],a['words'][0])
   elif t==5:event(8,m,vi);text=s['text'].split('\0')[0];different=text!=a['text'].split('\0')[0]
   elif t==8:event(6,m,vi,3);v[0]=sum(color(w[j])<<(8*j) for j in range(3));different=v[0]!=a['words'][0]
   elif t in [9,10,11]:
    n={9:2,10:3,11:4}[t];event(6,m,vi,n);different=any(not same_float(v[j],w[j]) for j in range(n));v[:n]=map(quiet,w[:n])
   elif t==14:event(7,m,vi,16);different=v!=w;v=list(map(quiet,w))
   allowed=not(a['flags']&0x20) and (q['phase'] in [0,1,2] or q['phase']==4 and not(a['flags']&0x400))
   if different and allowed:a['words']=v;a['text']=text;a['flags']|=0x4080
 def apply(h,m):
  for a in es[h-1]['attrs']:
   vi,s=find(m,a)
   if s is None:continue
   t=a['flags']&31;w=a['words'];data=[];kind=0
   if t in [2,3,4]:kind=10 if t==3 else 9;data=[int(bool(w[0]&255)) if t==4 else w[0]]
   elif t==5:
    text=a['text'].split('\0')[0];token=0 if not text else 101 if text=='texA' else 102 if text=='texB' else 103;event(13,token);event(14,m,vi,1,[token]);s['words'][0]=token;s['text']='' if token==0 else 'texA' if token==101 else 'texB' if token==102 else 'other';continue
   elif t==8:
    c=struct.unpack('<f',struct.pack('<I',0x3b808081))[0];data=[struct.unpack('<I',struct.pack('<f',((w[0]>>(8*j))&255)*c))[0] for j in range(3)];kind=11
   elif t in [9,10,11]:data=w[:{9:2,10:3,11:4}[t]];kind=11
   elif t==14:data=w;kind=12
   else:continue
   event(kind,m,vi,len(data),data);s['words'][:len(data)]=data
 def loop(src,backup):
  for i,h in enumerate(src):
   el=resolve(h)
   if not el or not ismat(el):continue
   m=acquire(el)
   if not m:continue
   if backup is not None and i<len(backup):
    b=resolve(backup[i])
    if b and not ismat(b):b=0
    capture(b,m)
   apply(el,m)
 def snapshot():
  out=bytearray()
  def text(s):
   b=s.encode();return struct.pack('<I',len(b))+b
  for m in ms:
   out+=struct.pack('<I',m['refs'])
   for v in m['vars']:out+=struct.pack('<16I',*v['words'])+text(v['text'])
  for e in es:
   out+=struct.pack('<I',e['cached'])
   for a in e['attrs']:out+=struct.pack('<17I',a['flags'],*a['words'])+text(a['text'].split('\0')[0] if a['flags']&31==5 else '')
  return out
 for _ in range(2 if q['flags']&4 else 1):
  loop(q['source'],q['backup'] if q['flags']&1 else None);snapshots.append(snapshot())
  if q['flags']&3==3:loop(q['backup'],None);snapshots.append(snapshot())
 return q,events,snapshots
def expected(cases):
 out=bytearray(struct.pack('<I',len(cases)));counts={};details=[]
 for i,q in enumerate(cases):
  final,events,snaps=simulate(q);out+=struct.pack('<II',i,len(snaps))+b''.join(snaps)+struct.pack('<I',len(events))
  for e in events:out+=struct.pack('<20I',*e);counts[str(e[0])]=counts.get(str(e[0]),0)+1
  if i<4:details.append(dict(label=q['label'],final_material1_word0=final['materials'][0]['vars'][0]['words'][0],final_material2_word0=final['materials'][1]['vars'][0]['words'][0],final_cache=[e['cached'] for e in final['elements']],events=len(events)))
 return bytes(out),counts,details
def main():
 pre=json.loads((OUT/'preflight.json').read_text());need(all(sha(n)==h for n,h in pre['protected'].items()),'Protected inputs preflight');need(sha(pre['parent'])==pre['parent_sha256']=='72d3eff2fcef83d8b4f2ea1d361efaff6289645c617e9adac031a02e1a9a9814','Accepted parent')
 need(sha(IFM)=='8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4','Historical IFM identity')
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 _,static=check_ranges(IFM,'ifm-disassembly.txt',[('whole_collection_capture_apply',0x1023e3e0,0x1023e4b4),('whole_lazy_material_acquisition',0x105c1220,0x105c1260),('reference_assignment',0x10984a60,0x10984a9f),('whole_typed_capture',0x1023c0a0,0x1023c286),('whole_typed_apply',0x1023e140,0x1023e3a4)])
 (run/'static-reference.json').write_text(json.dumps(static,indent=2)+'\n');parent=json.loads(Path(pre['parent']).read_text());sources=source_closure([Path(n) for n in parent['source']]+[ROOT/'work'/n for n in NEW])
 start=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_session_collection.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180);(run/'build-driver.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'Fresh collection reference/owned compile')
 exes=[]
 for tag,machine in [('reference',0x14c),('owned',0x8664),('safety',0x8664)]:
  exe=OUT/('collection_'+tag+('_x86.exe' if tag=='reference' else '_x64.exe'));raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];need(exe.stat().st_mtime>=start-2 and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'Fresh architecture/asInvoker '+tag)
  log=(OUT/(tag+'-build.log')).read_text();need(not re.search(r'\b(?:warning|error)\b',log,re.I),'Clean compile '+tag)
  imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(tag+'-imports.txt')).write_text(imports);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);need(modules==['KERNEL32.dll'],'Only system imports '+tag);exes.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),imports=modules))
 cases=corpus();query=run/'query.bin';query.write_bytes(encode(cases));predicted,counts,witnesses=expected(cases);(run/'expected.bin').write_bytes(predicted);(run/'case-labels.json').write_text(json.dumps([q['label'] for q in cases],indent=2)+'\n')
 need(witnesses[0]['final_material1_word0']==11 and witnesses[1]['final_material1_word0']==11 and witnesses[1]['final_material2_word0']==7,'Alias and mismatched-resource independent witnesses')
 for tag,exe in zip(['reference','owned'],exes[:2]):
  target=run/(tag+'.bin');p=subprocess.run([exe['path'],str(IFM if tag=='reference' else run/'absent-engine.dll'),str(query),str(target)],cwd=run,capture_output=True,text=True,timeout=90);(run/(tag+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,tag+' child exit '+str(p.returncode));actual=target.read_bytes()
  if actual!=predicted:
   first=next((i for i,(a,b) in enumerate(zip(actual,predicted)) if a!=b),min(len(actual),len(predicted)));raise RuntimeError(tag+' mismatch byte '+str(first)+' expected '+str(len(predicted))+' actual '+str(len(actual)))
 safety=subprocess.run([exes[2]['path']],cwd=run,capture_output=True,text=True,timeout=30);(run/'safety.log').write_text(safety.stdout+safety.stderr);need(safety.returncode==0 and 'collection_safety_checks=25' in safety.stdout,'25 resource/collection safety checks')
 need(all(sha(n)==h for n,h in pre['protected'].items()) and all(sha(n)==h for n,h in sources.items()),'Protected inputs and complete source closure unchanged')
 need(sha(pre['editor'])==pre['editor_sha256'] and all(sha(ROOT/n)==h for n,h in pre['editor_dependencies'].items()),'Editor and312dependencies unchanged')
 result=dict(status='VERIFIED collection ordering and controlled state; application INCOMPLETE',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',cases=len(cases),ordered_events=sum(counts.values()),event_kinds=counts,exact_output_bytes=len(predicted),safety=safety.stdout.strip(),witnesses=witnesses,executables=exes,query_sha256=sha(query),output_sha256=sha(run/'reference.bin'),source=sources,protected=pre['protected'],parent=pre['parent'],parent_sha256=pre['parent_sha256'],static_reference=str(run/'static-reference.json'),static_reference_sha256=sha(run/'static-reference.json'),editor=pre['editor'],editor_sha256=pre['editor_sha256'],editor_dependencies=pre['editor_dependencies'],editor_rebuilt=False,limits=['Whole original collection, lazy acquisition, attribute capture/setters and application execute unchanged RX; material variables/resources/type checks/handle resolver are explicit host services','No real CMaterialVar conversion/type replacement, shader/proxy lazy initialization, texture resource ownership or DrawModel execution','No production integration, GPU/visual fidelity, native loaded attribute chain or real benchmark material lifecycle claim','Collection helper is sequential, not atomic; caller must stage failed operations. Backup replay is not universal original-state rollback','Historical private IFM differs from externally modified installed IFM; originals read-only and live SFM unused'])
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','ordered_events','exact_output_bytes','witnesses']},indent=2),flush=True)
if __name__=='__main__':main()
