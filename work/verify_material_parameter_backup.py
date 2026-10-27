"""Staged original MaterialSystem GetString -> original IFM capture vs owned64.

Original images have the same preferred base. They execute in separate processes;
the exact captured byte strings are pinned at that boundary. No combined engine
resource lifetime, native undo/notification reconstruction or renderer is claimed.
"""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,re,struct,subprocess,tempfile,time
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges
from verify_material_get_string import corpus,packet

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/material-typed-backup-composition-20261027'
LIB=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/materialsystem.dll'
IFM=LIB.parent/'tools/ifm.dll'
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
NEW=['material_parameter_backup_candidate.h','probe_material_parameter_backup.cpp','test_material_parameter_backup.cpp','build_material_parameter_backup.cmd','verify_material_parameter_backup.py']
def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def need(b,s):
 if not b:raise RuntimeError(s)
def text(s):return struct.pack('<I',len(s))+s
def decode_getter(raw):
 at=0
 def word():
  nonlocal at
  n=struct.unpack_from('<I',raw,at)[0];at+=4;return n
 def string():
  nonlocal at
  n=word();s=raw[at:at+n];at+=n;need(len(s)==n,'boundary text extent');return s
 rows=[]
 for _ in range(word()):
  storage=word();snapshot=string();after=string();counts=[word() for _ in range(4)];rows.append(dict(storage=storage,snapshot=snapshot,after=after,counts=counts))
 need(at==len(raw),'complete native getter boundary');return rows
def templates(boundary):
 cases=[]
 def row(index,slot=0,metadata=0,equal=False,name=None):
  return dict(flags=5|metadata,index=index,name=name or ('$p_'+str(slot)).encode(),old=boundary[index]['snapshot'] if equal else b'old',before=[0x12340000+i for i in range(16)])
 # Every admitted getter source is consumed as changed AND equal text.
 for i in range(len(boundary)):
  cases.extend([(3,0,[row(i)]),(3,0,[row(i,equal=True)])])
 # Full ordered multi-variable passes; first getter text must survive all later
 # getters. These use original snapshots copied before native scratch reuse.
 for start in range(0,len(boundary),32):
  ids=range(start,min(start+32,len(boundary)))
  cases.append((3,0,[row(i,j) for j,i in enumerate(ids)]))
  cases.append((3,0,[row(i,j,equal=True) for j,i in enumerate(ids)]))
 chosen=sorted(set(list(range(min(80,len(boundary))))+[len(boundary)-1]))
 for i in chosen:
  for phase in [-1,0,1,2,3,4,5]:
   for meta in [0,0x20,0x400,0x80,0x4080,0x2080,0xffff0000]:cases.append((3,phase,[row(i,metadata=meta)]))
 for flags in range(4):cases.append((flags,0,[row(i,j) for j,i in enumerate(chosen[:16])]))
 cases.append((3,0,[row(0,name=b'$FLAGS'),row(1,1,name=b'$flags2'),row(2,2,name=b'plain')]))
 return cases
def encode(cases,boundary,poison=False):
 data=bytearray(struct.pack('<I',len(cases)))
 for flags,phase,rows in cases:
  data+=struct.pack('<III',flags,phase&0xffffffff,len(rows))
  for row in rows:
   live=[0]*15+[row['index']];data+=struct.pack('<35I',row['flags'],3,0,*row['before'],*live)
   data+=text(row['name'])+text(row['old'])+text(b'POISON: owned must derive text from typed storage' if poison else boundary[row['index']]['snapshot'])
 return data
def predict(cases,boundary):
 output=bytearray(struct.pack('<I',len(cases)));events=mutations=phase_calls=0
 for ci,(flags,phase,rows) in enumerate(cases):
  output+=struct.pack('<II',ci,len(rows));trace=[]
  for i,row in enumerate(rows):
   f=row['flags'];old=row['old'].split(b'\0')[0];value=old;name=row['name'].split(b'\0')[0]
   if flags&3==3 and name.startswith(b'$') and name.lower() not in [b'$flags',b'$flags2']:
    trace.extend([(0,i,0),(5,i,0)]);next_text=boundary[row['index']]['snapshot'].split(b'\0')[0];different=old!=next_text
    if different and not(f&0x20):phase_calls+=1
    if different and not(f&0x20) and (phase in [0,1,2] or phase==4 and not(f&0x400)):
     f|=0x4080;value=next_text;mutations+=1
   output+=struct.pack('<17I',f,*row['before'])+text(value)
  output+=struct.pack('<I',len(trace))
  for event in trace:output+=struct.pack('<III',*event)
  events+=len(trace)
 return bytes(output),dict(events=events,mutations=mutations,phase_calls=phase_calls)

def main():
 pre=json.loads((OUT/'preflight.json').read_text())
 need(sha(pre['parent'])==pre['parent_sha256']=='d10683f09bda7ce6887660b60365009c85c562413941272e50b88228377f65cd','accepted getter receipt')
 need(all(sha(p)==h for p,h in pre['protected'].items()),'all preceding inputs/sources unchanged before')
 need(sha(LIB)=='c78e414891b4d87e3922610a8bf937bca3efbaab95bced488522045998bc87f8','material image identity')
 need(sha(IFM)=='8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4','historical IFM identity')
 parent=json.loads(Path(pre['parent']).read_text());sources=source_closure([Path(p) for p in parent['source']]+[ROOT/'work'/n for n in NEW])
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 _,getter_ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[('whole_GetString',0x10017de0,0x10018045),('CRT_float_format_init',0x1007d4e7,0x1007d547)])
 _,capture_ranges=check_ranges(IFM,'ifm-disassembly.txt',[('whole_capture',0x1023c0a0,0x1023c286),('intern_then_set',0x1023b3b0,0x1023b3de),('string_guard',0x10365ba0,0x10365c00),('string_setter',0x10366100,0x103661c9)])
 (run/'static-reference.json').write_text(json.dumps(dict(material=getter_ranges,ifm=capture_ranges),indent=2)+'\n')
 started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_parameter_backup.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180)
 (run/'build-driver.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'fresh four probe builds and existing trig helper')
 exes=[]
 for tag,name,machine in [('getter','getter_reference_x86',0x14c),('capture','capture_reference_x86',0x14c),('owned','parameter_backup_owned_x64',0x8664),('safety','parameter_backup_safety_x64',0x8664)]:
  exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
  need(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'fresh architecture/manifest '+tag)
  log=(OUT/(tag+'-build.log')).read_text();(run/(tag+'-build.log')).write_text(log);need(not re.search(r'\b(?:warning|error)\b',log,re.I),'clean build '+tag)
  inspect=subprocess.run([str(DUMP),'/headers','/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(tag+'-inspection.txt')).write_text(inspect)
  modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',inspect,re.M|re.I);need(modules==['KERNEL32.dll'],'system-only direct imports '+tag)
  exes.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),imports=modules))
 def execute(args,label):
  p=subprocess.run(list(map(str,args)),cwd=run,capture_output=True,text=True,timeout=120);(run/(label+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,label+' failed: '+p.stderr);return p.stdout
 safety=execute([exes[3]['path']],'safety');need('store_initialized_and_retired=1' in safety,'actual store lifecycle integration')
 # Unsupported raw storage kinds have no owned retained-value representation.
 # They remain explicitly refused; all other existing getter corpus rows stay.
 original=corpus();admitted=[c for c in original if c[0] in [0,1,2,3,4,6,7,8]]
 query=bytearray(struct.pack('<I',len(admitted)));expected_getter=bytearray(query)
 for c in admitted:query+=struct.pack('<21I',*c[:5],*c[5])+text(c[6])+text(c[7]);expected_getter+=packet(c)
 (run/'sources.bin').write_bytes(query)
 native_log=execute([exes[0]['path'],LIB,run/'sources.bin',run/'getter.bin'],'getter')
 need((run/'getter.bin').read_bytes()==expected_getter,'entire actual getter vs independent decimal model')
 boundary=decode_getter((run/'getter.bin').read_bytes());cases=templates(boundary)
 (run/'capture-query.bin').write_bytes(encode(cases,boundary));(run/'owned-query.bin').write_bytes(encode(cases,boundary,True));predicted,counts=predict(cases,boundary);(run/'expected.bin').write_bytes(predicted)
 capture_log=execute([exes[1]['path'],IFM,run/'capture-query.bin',run/'capture.bin'],'capture')
 execute([exes[2]['path'],run/'absent-engine.dll',run/'sources.bin',run/'owned-query.bin',run/'owned.bin'],'owned')
 for name in ['capture','owned']:
  actual=(run/(name+'.bin')).read_bytes()
  if actual!=predicted:
   at=next((i for i,(a,b) in enumerate(zip(actual,predicted)) if a!=b),min(len(actual),len(predicted)));raise RuntimeError(name+' comparison mismatch at byte '+str(at))
 need('phase_calls='+str(counts['phase_calls'])+' undo_disabled_calls='+str(counts['mutations'])+' notifications_suppressed='+str(counts['mutations']) in capture_log,'native setter/mutation counts exact')
 m=re.search(r'allocations=(\d+) releases=(\d+)',native_log);need(m and m[1]==m[2],'getter transient allocations balanced')
 need(all(sha(p)==h for p,h in sources.items()) and all(sha(p)==h for p,h in pre['protected'].items()),'all source/input identities unchanged after')
 need(sha(pre['editor'])==pre['editor_sha256'] and all(sha(ROOT/p)==h for p,h in pre['editor_dependencies'].items()),'current publication editor313 dependencies unchanged')
 result=dict(status='VERIFIED staged typed-to-string backup composition; PARTIALLY VERIFIED full material lifecycle/application',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',source_cases=len(admitted),refused_unknown_storage_cases=len(original)-len(admitted),capture_cases=len(cases),capture_attributes=sum(len(rows) for _,_,rows in cases),exact_output_bytes=len(predicted),**counts,safety=safety.strip(),getter_allocations=int(m[1]),getter_releases=int(m[2]),shared_scratch_overwrite_cases=sum(v['storage']==2 and v['snapshot']!=v['after'] for v in boundary),source=sources,protected=pre['protected'],parent=pre['parent'],parent_sha256=pre['parent_sha256'],executables=exes,boundary=dict(source_query_sha256=sha(run/'sources.bin'),actual_getter_sha256=sha(run/'getter.bin'),native_capture_query_sha256=sha(run/'capture-query.bin'),owned_poisoned_query_sha256=sha(run/'owned-query.bin'),native_capture_output_sha256=sha(run/'capture.bin'),owned_output_sha256=sha(run/'owned.bin')),editor=pre['editor'],editor_sha256=pre['editor_sha256'],editor_dependencies=pre['editor_dependencies'],editor_rebuilt=False,publication_rebuild=pre['publication_rebuild'],limits=['Original MaterialSystem and IFM run in separate processes with pinned byte transport, not one live native engine chain','Actual original GetString then actual IFM string intern/setter; host FindVar/name/phase/disabled-undo/suppressed-notify services remain explicit','Resource3/8 source values are controlled retained layout/name projections, not real texture/material ownership or native constructors','Owned store construction/initialization/uncache/destruction tested with authored VertexLitGeneric alpha/color; broader shaders and native resource lifetime not proven','String-target composition compared with original; additional mixed cached typed targets have owned safety checks only, not new native end-to-end comparison','Unknown raw storage5/9..15 refused, not silently mapped to empty text','No document edit/history integration, model/draw restoration, renderer, GPU, SFM process/computer control, original writes or publication'])
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n')
 print(json.dumps({k:result[k] for k in ['status','source_cases','capture_cases','capture_attributes','exact_output_bytes','getter_allocations','safety']},indent=2),flush=True)
if __name__=='__main__':main()
