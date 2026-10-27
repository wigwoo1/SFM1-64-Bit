"""Unchanged private material factory/member/string-copy and owned x64 contract.

Explicit canonical string, undo/phase/notification, heap and resource services.
No original process, imports/DllMain, instruction replacements or original writes.
"""
import hashlib,json,re,struct,subprocess,tempfile,time
from pathlib import Path
from datetime import datetime,timezone
from verify_ifm_draw_packet import check_ranges
from verify_builtin_error_material_seed import source_closure
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/material-element-binding-20261027'
IFM=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/tools/ifm.dll'
PARENT=ROOT/'outputs/material-session-backup-construction-20261027/verification-ixdnws6k/results.json'
NEW=['material_element_binding_candidate.h','probe_material_element_binding.cpp','build_material_element_binding.cmd','verify_material_element_binding.py']
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def need(b,s):
 if not b:raise RuntimeError(s)
def corpus():
 return [(a,b,p,f,available,clear,lookup) for a,b in [(1,1),(1,2),(2,1),(0,1),(1,0),(3,2)] for p in range(7) for f in [0x205,0x225,0x285,0x605,0x4285,0x4205,0x1205] for available in [0,1] for clear in [0,1] for lookup in [0,1]]
def expected(cases):
 output=bytearray(struct.pack('<I',len(cases)));events_count=0;witnesses=[]
 for initial,replacement,phase,flags,available,clear,lookup in cases:
  events=[];refs=[0,0,0]
  elements=[dict(name=initial,cache=0,flags=0x4205),dict(name=initial,cache=0,flags=0x4285)]
  def acquire(e):
   if e['cache']:return
   token=1 if lookup else e['name'] if e['name'] in [1,2] else 0
   events.append((0,token));e['cache']=token
   if token:events.append((1,token));refs[token]+=1
  def resolve(e,available):
   if e['flags']&0x80 and e['cache'] and available:events.append((2,e['cache']));refs[e['cache']]-=1;e['cache']=0
  def snapshot():
   for e in elements:output.extend(struct.pack('<5I',e['name'],e['cache'],e['flags'],0x4085,2))
  acquire(elements[0]);acquire(elements[1]);snapshot()
  e=elements[0];e['flags']=flags
  if initial!=replacement and not(flags&0x20) and (phase in [0,1,2] or phase==4 and not(flags&0x400)):e['name']=replacement;e['flags']|=0x4080
  snapshot();resolve(e,available);snapshot();acquire(e);snapshot()
  if clear:e['flags']&=~0x80
  resolve(e,True);acquire(e);snapshot()
  output.extend(struct.pack('<4I',*refs,len(events)))
  for event in events:output.extend(struct.pack('<2I',*event))
  events_count+=len(events)
  if (initial,replacement,phase,flags,available,clear,lookup) in [(1,2,0,0x205,1,0,0),(1,2,0,0x205,0,1,0),(1,2,0,0x205,1,1,1)]:witnesses.append(dict(query=[initial,replacement,phase,flags,available,clear,lookup],events=events,final=elements,refs=refs))
 return bytes(output),events_count,witnesses
def main():
 OUT.mkdir(exist_ok=True);need(sha(PARENT)=='5dfd6915f36188321d8bc5a3d94f8ba7c056faba4f977737affa24bab1156550','accepted parent receipt')
 parent=json.loads(PARENT.read_text());protected=parent['protected']|parent['source'];need(all(sha(n)==h for n,h in protected.items()),'preceding sources/protected inputs unchanged before')
 need(sha(IFM)=='8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4','private original identity')
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 ranges=[('factory',0x105c13b0,0x105c1416),('base_constructor',0x10361c40,0x10361d26),('OnConstruction',0x105c1380,0x105c13a1),('member_attribute_creation',0x103615e0,0x10361701),('dynamic_attribute_creation',0x103614f0,0x103615d5),('member_attribute_factory',0x10365040,0x10365077),('dynamic_attribute_factory',0x10365000,0x10365033),('member_attribute_constructor',0x10363350,0x103633d6),('dynamic_attribute_constructor',0x103632c0,0x10363348),('string_initialize',0x10361790,0x103617a0),('string_allocate',0x10363e10,0x10363e34),('string_copy',0x103667f0,0x1036680f),('string_set',0x10366100,0x103661c9),('name_set',0x105c11c0,0x105c11ef),('resolve',0x105c11f0,0x105c1209),('name_get',0x105c1210,0x105c121e),('resource_get',0x105c1220,0x105c1260),('cache_release',0x10984960,0x10984999),('cache_assign',0x10984a60,0x10984a9f),('cache_construct',0x10984c40,0x10984c6a),('copy_context',0x103621f0,0x103622b5),('attribute_traversal',0x10361d30,0x10361ead),('type5_registration',0x10a6a580,0x10a6a5a5)]
 image,static=check_ranges(IFM,'ifm-disassembly.txt',ranges)
 need(image.string(0x10ae4fe4)=='mtlName' and image.word(0x10b37a14+8)==0x10361790 and image.word(0x10b37a14+32)==0x103667f0,'string vtable and actual name identity')
 need(image.word(0x10b942b8+12)==0x105c11f0 and image.word(0x10b942b8+40)==0x105c1380,'material resolve/construction virtuals')
 (run/'static-reference.json').write_text(json.dumps(static,indent=2)+'\n')
 sources=source_closure([Path(n) for n in parent['source']]+[ROOT/'work'/n for n in NEW]);start=time.time()
 build=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_element_binding.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180);(run/'build.log').write_text(build.stdout+build.stderr);need(build.returncode==0,'fresh build failed: inspect build logs')
 exes=[]
 for tag,machine in [('reference',0x14c),('owned',0x8664)]:
  exe=OUT/('element_binding_'+tag+('_x86.exe' if tag=='reference' else '_x64.exe'));raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];need(exe.stat().st_mtime>=start-2 and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'fresh architecture/manifest')
  log=(OUT/(tag+'-build.log')).read_text();need(not re.search(r'\b(?:warning|error)\b',log,re.I),'clean build '+tag)
  imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(tag+'-imports.txt')).write_text(imports);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);need(modules==['KERNEL32.dll'],'system-only direct imports');exes.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),imports=modules))
 cases=corpus();query=run/'query.bin';query.write_bytes(struct.pack('<I',len(cases))+b''.join(struct.pack('<7I',*q) for q in cases));predicted,event_count,witnesses=expected(cases);(run/'expected.bin').write_bytes(predicted)
 for tag,exe in zip(['reference','owned'],exes):
  target=run/(tag+'.bin');p=subprocess.run([exe['path'],str(IFM if tag=='reference' else run/'absent.dll'),str(query),str(target)],cwd=run,capture_output=True,text=True,timeout=90);(run/(tag+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,tag+' exit '+str(p.returncode)+' '+p.stderr)
  actual=target.read_bytes()
  if actual!=predicted:
   first=next((i for i,(a,b) in enumerate(zip(actual,predicted)) if a!=b),min(len(actual),len(predicted)));raise RuntimeError(tag+' mismatch byte '+str(first))
 safety=subprocess.run([exes[1]['path'],'--self-test'],cwd=run,capture_output=True,text=True,timeout=30);(run/'safety.log').write_text(safety.stdout+safety.stderr);need(safety.returncode==0 and 'element_binding_safety_checks=11' in safety.stdout,'11 owned safety checks')
 need(all(sha(n)==h for n,h in protected.items()) and all(sha(n)==h for n,h in sources.items()),'protected sources unchanged after')
 need(sha(parent['editor'])==parent['editor_sha256'] and all(sha(ROOT/n)==h for n,h in parent['editor_dependencies'].items()),'editor and312dependencies unchanged')
 result=dict(status='VERIFIED material construction/string binding contract; PARTIALLY VERIFIED lifecycle',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',cases=len(cases),events=event_count,exact_output_bytes=len(predicted),safety=safety.stdout.strip(),witnesses=witnesses,executables=exes,parent=str(PARENT),parent_sha256=sha(PARENT),source=sources,protected=protected,static_reference_sha256=sha(run/'static-reference.json'),query_sha256=sha(query),output_sha256=sha(run/'reference.bin'),editor=parent['editor'],editor_sha256=parent['editor_sha256'],editor_dependencies=parent['editor_dependencies'],editor_rebuilt=False,limits=['actual factory and original type5 initialize/allocate/copy/set execute unchanged RX','prepopulated allocator freelists; private heap for dynamic string payload/copy context; real pool growth and GC not verified','explicit DataModel canonical interning, disabled undo, suppressed notifications, phase and MaterialSystem lookup/refcount services','two separately constructed DmeMaterial roots may acquire identical resource tokens; real FindMaterial implementation not executed','no reference-graph clone, production integration, GPU, DrawModel or benchmark lifecycle claim'])
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','events','exact_output_bytes','witnesses']},indent=2),flush=True)
if __name__=='__main__':main()
