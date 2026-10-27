"""Original FindMaterial miss/file/parse/raw construction versus owned request policy.

Memory-only filesystem, fixed valid VMT, tools mode false. No shader initialization,
real material variables, assets, original writes, DllMain or live SFM. Requests must
fit the original 260-byte buffer including NUL; longer inputs are safely refused.
"""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,random,re,struct,subprocess,tempfile,time
from verify_ifm_draw_packet import check_ranges
from verify_builtin_error_material_seed import source_closure

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/material-find-uncached-20261027'
PARENT=ROOT/'outputs/material-find-cached-20261027/verification-aihox4s9/results.json'
LIB=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/materialsystem.dll'
SESSION=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\elements\sessions\mtt_soldier\mtt_soldier.dmx')
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
NEW=['material_find_file_request_candidate.h','probe_material_find_uncached.cpp',
     'build_material_find_uncached.cmd','verify_material_find_uncached.py']

def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()

def need(b,s):
 if not b:raise RuntimeError(s)

def key(s):
 s=s.split(b'\0')[0].lower()
 # Last period terminates the suffix only if reached before a path separator;
 # a period in position zero is a name rather than a removable extension.
 end=len(s)-1
 while end>0 and s[end] not in b'./\\':end-=1
 if end>0 and s[end:end+1]==b'.':s=s[:end]
 return s.replace(b'\\',b'/')

def request(s):
 k=key(s);brace=k.startswith(b'{')
 filename=(b'' if brace else b'materials/')+k+b'.vmt'
 need(len(s)<=4095 and len(filename)<260,'safe request scope')
 return k,(filename,b'rb',b'' if brace else b'GAME')

def pack_text(s):return struct.pack('<I',len(s))+s

def predict(case):
 cache={};requests=[];created=[];ids=[]
 for name in case['names']:
  k,req=request(name)
  if k in cache:ids.append(cache[k]);continue
  requests.append(req)
  if not case['available']:ids.append(0);continue
  # Factory receives full normalized name + .vmt, not the file request or
  # request-buffer truncation. It strips that extension again. Empty -> .vmt.
  stored=key(k+b'.vmt');created.append(stored);ident=len(created)
  cache.setdefault(stored,ident);ids.append(ident)
 out=struct.pack('<III',*ids,len(requests))
 for req in requests:
  for value in req:out+=pack_text(value)
 out+=struct.pack('<4I',*([len(requests) if case['available'] else 0]*4))
 for name in created:out+=pack_text(name)
 out+=struct.pack('<I',0xffffffff)
 return out,dict(ids=ids,requests=len(requests),created=len(created))

def corpus(benchmark):
 rows=[]
 def add(label,a,b):
  for available in [0,1]:rows.append(dict(label=label,names=[a,b],available=available))
 values=[b'',b'.',b'..',b'.Hidden',b'dir/.Hidden',b'dir/..',b'a.',b'A/B',
  b'A\\B.VMT',b'A/B.PNG',b'A/B.Foo.Bar',b'Dir.Name/Leaf',b'/ABSOLUTE/NAME.VMT',
  b'C:\\PATH\\FILE.VMT',b'../Relative.VMT',b'foo//BAR.vmt',b'foo/./bar.VMT',
  b'{Procedural}.VMT',b'INPUT.VMT\0IGNORED']
 for name in values:
  add('same-name',name,name)
  add('case-and-slash-pair',name,name.upper().replace(b'/',b'\\'))
  add('second-normalization',name,key(name))
 for base in [b'fixture/a',b'fixture/path.with.dot/name',b'fixture/.file',b'fixture/name.']:
  for extension in [b'',b'.vmt',b'.VMT',b'.png',b'.x.y']:
   for upper in [False,True]:
    for backslash in [False,True]:
     name=(base+extension).upper() if upper else base+extension
     name=name.replace(b'/',b'\\') if backslash else name
     add('extension-case-separator',name,base)
 for name in benchmark:
  value=name.encode('utf-8')
  add('benchmark-repeat',value,value)
  add('benchmark-case',value,value.upper())
  add('benchmark-extension-slash',value,value.replace(b'/',b'\\')+b'.VMT')
  if value.startswith(b'/'):add('benchmark-leading-slash-distinct',value,value[1:])
 for name in [b'fixture/a',b'fixture/a.vmt',b'{fixture/a}',b'foo//bar',b'foo/../bar']:
  add('distinct-leading-slash',b'/'+name,name)
 for size in [243,244,245]:add('safe-buffer-boundary',b'A'*size,b'a'*size)
 for size in [253,254,255]:add('safe-brace-buffer-boundary',b'{'+b'B'*(size-1),b'{'+b'b'*(size-1))
 # Same OS request after slash/case normalization; do not collapse resource IDs
 # with a leading slash or dot/repeated-separator components.
 rng=random.Random(2026102743)
 alphabet=b'abcXYZ012_-/\\.{}:'
 for i in range(512):
  name=bytes(rng.choice(alphabet) for _ in range(rng.randrange(1,100)))
  second=name.upper().replace(b'/',b'\\') if i%3==0 else b'/'+name if i%3==1 else name+b'.PNG'
  add('seeded-names',name,second)
 return rows

def main():
 OUT.mkdir(exist_ok=True)
 need(sha(PARENT)=='26e982e6b5d73aa04c07e7263430e35ad2738cadbed4a2dae1bcb75076dd36c3','accepted parent receipt')
 parent=json.loads(PARENT.read_text());protected=parent['protected']|parent['source']
 need(all(sha(p)==h for p,h in protected.items()),'protected inputs before')
 need(sha(LIB)=='c78e414891b4d87e3922610a8bf937bca3efbaab95bced488522045998bc87f8','material image')
 need(sha(SESSION)==parent['benchmark_sha256'],'original benchmark read-only identity')
 audit=PARENT.parent/'benchmark-audit.json'
 need(sha(audit)==parent['benchmark_audit_sha256'],'accepted all-graph benchmark names')
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 ranges=[('whole_FindMaterial77',0x10016f60,0x100172a0),
  ('load_VMT_request',0x10006ba0,0x10006cb4),('KV_file_read_parse',0x10071da0,0x10071ef4),
  ('format_request',0x1006e590,0x1006e5bc),('raw_factory',0x10006cc0,0x10006d1a),
  ('raw_constructor',0x10006650,0x10006784),('dictionary_factory_insert',0x10009b50,0x10009b7b),
  ('raw_deleting_destructor',0x10006d20,0x10006d43),('pool_allocate',0x10070390,0x100703de),
  ('pool_release',0x100702b0,0x100702c9)]
 image,static=check_ranges(LIB,'materialsystem-disassembly.txt',ranges)
 for address,value in [(0x10095fcc,'materials/%s.vmt'),(0x10095fc4,'%s.vmt'),
  (0x10098738,'materials/'),(0x10095ce4,'.vmt'),(0x100955d4,'GAME'),(0x10095d6c,'rb')]:
  need(image.string(address)==value,'literal '+hex(address))
 need(image.word(0x100984cc+77*4)==0x10016f60 and image.word(0x10095d74+94*4)==0x100067b0,'actual virtual identities')
 (run/'static-reference.json').write_text(json.dumps(static,indent=2)+'\n')
 sources=source_closure([Path(p) for p in parent['source']]+[ROOT/'work'/p for p in NEW])
 started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_find_uncached.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180)
 (run/'build.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'fresh build failed')
 exes=[]
 for tag,machine in [('reference',0x14c),('owned',0x8664)]:
  exe=OUT/('find_uncached_'+tag+('_x86.exe' if tag=='reference' else '_x64.exe'))
  raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
  need(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'fresh architecture/manifest')
  log=(OUT/(tag+'-build.log')).read_text();(run/(tag+'-build.log')).write_text(log)
  need(not re.search(r'\b(?:warning|error)\b',log,re.I),'clean build '+tag)
  imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout
  (run/(tag+'-imports.txt')).write_text(imports);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I)
  need(modules==['KERNEL32.dll'],'system-only direct imports')
  exes.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),imports=modules))
 materials=json.loads(audit.read_text())['materials'];cases=corpus([m['name'] for m in materials.values()])
 query=struct.pack('<I',len(cases));expected=bytearray(query);details=[]
 for c in cases:
  query+=b''.join(pack_text(v) for v in c['names'])+struct.pack('<I',c['available'])
  output,detail=predict(c);expected+=output;details.append(dict(label=c['label'],available=c['available'],**detail))
 (run/'query.bin').write_bytes(query);(run/'expected.bin').write_bytes(expected)
 (run/'case-coverage.json').write_text(json.dumps(details,indent=2)+'\n')
 for tag,exe in zip(['reference','owned'],exes):
  target=run/(tag+'.bin')
  p=subprocess.run([exe['path'],str(LIB if tag=='reference' else run/'absent.dll'),str(run/'query.bin'),str(target)],cwd=run,capture_output=True,text=True,timeout=90)
  (run/(tag+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,tag+' exit '+str(p.returncode)+' '+p.stderr)
  actual=target.read_bytes()
  if actual!=expected:
   at=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
   raise RuntimeError(tag+' mismatch byte '+str(at))
 native=(run/'reference.log').read_text();m=re.search(r'allocations=(\d+) releases=(\d+)',native)
 need(m and m[1]==m[2],'native tracked heap cleanup')
 safety=subprocess.run([exes[1]['path'],'--self-test'],cwd=run,capture_output=True,text=True,timeout=30)
 (run/'safety.log').write_text(safety.stdout+safety.stderr)
 need(safety.returncode==0 and 'checks=10' in safety.stdout,'ten owned safety checks')
 # Execute the same original formatter in a fixture-owned buffer with explicit
 # guards. Inspect bytes directly, never strlen/read beyond a nonterminated260.
 boundary=run/'format-boundary.bin'
 p=subprocess.run([exes[0]['path'],str(LIB),'--format-boundary',str(boundary)],cwd=run,capture_output=True,text=True,timeout=30)
 (run/'format-boundary.log').write_text(p.stdout+p.stderr)
 need(p.returncode==0 and 'cases=18' in p.stdout,'native controlled formatting boundary')
 format_expected=bytearray(struct.pack('<I',18));unterminated=0;truncated=0
 for brace in [False,True]:
  for size in [244,245,246,247,254,255,256,257,4095]:
   name=(b'{'+b'a'*(size-1)) if brace else b'a'*size
   full=(b'' if brace else b'materials/')+name+b'.vmt';storage=bytearray(b'Z'*272)
   storage[:min(260,len(full))]=full[:260]
   if len(full)<260:storage[len(full)]=0
   elif len(full)>260:storage[259]=0;truncated+=1
   else:unterminated+=1
   format_expected+=struct.pack('<III',size,int(brace),min(260,len(full)))+storage
 need(boundary.read_bytes()==format_expected,'original formatting buffer/return/guard bytes')
 boundary_evidence=dict(cases=18,unterminated_exact_buffer_cases=unterminated,truncated_cases=truncated,
  output_sha256=sha(boundary),guard='12 fixture bytes unchanged; no C-string read of full destination',heap_cleanup=p.stdout.strip())
 need(all(sha(p)==h for p,h in protected.items()) and all(sha(p)==h for p,h in sources.items()),'sources/protected inputs unchanged after')
 need(sha(SESSION)==parent['benchmark_sha256'],'benchmark unchanged after')
 need(sha(parent['editor'])==parent['editor_sha256'] and all(sha(ROOT/p)==h for p,h in parent['editor_dependencies'].items()),'editor312deps unchanged')
 result=dict(status='VERIFIED bounded uncached FindMaterial request/cache/raw lifetime; PARTIALLY VERIFIED full material loading',
  utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',cases=len(cases),exact_output_bytes=len(expected),
  successful_loads=sum(v['created'] for v in details),file_requests=sum(v['requests'] for v in details),
  missing_file_requests=sum(v['requests'] for v in details if not v['available']),
  distinct_resources=sum(v['ids']==[1,2] for v in details),
  allocations=int(m[1]),releases=int(m[2]),safety=safety.stdout.strip(),benchmark_materials=len(materials),
  benchmark_audit=str(audit),benchmark_audit_sha256=sha(audit),benchmark_sha256=sha(SESSION),
  native_boundary_observation=boundary_evidence,
  source=sources,protected=protected,parent=str(PARENT),parent_sha256=sha(PARENT),executables=exes,
  static_reference_sha256=sha(run/'static-reference.json'),query_sha256=sha(run/'query.bin'),output_sha256=sha(run/'reference.bin'),
  editor=parent['editor'],editor_sha256=parent['editor_sha256'],editor_dependencies=parent['editor_dependencies'],editor_rebuilt=False,
  limits=['Actual original empty-dictionary FindMaterial77 executes file-loader/KV parser, raw constructor/insertion, cache hit and real destructor; fixed virtual UnlitGeneric VMT only',
   'Filesystem open/read/free, error-return resource, context/lock/TLS, allocation and KV symbol services explicit; no real assets or runtime DLL loading',
   'Tools mode false: parsed KV roots are destroyed, not used to initialize shader/material variables; no rendering fidelity claim',
   'Only requests of at most259 bytes, including prefix/extension but excluding NUL, are admitted; native260-byte unterminated and longer truncated requests safely rejected by owned policy',
   'Original all-graph benchmark material names replayed with virtual content, not original VMTs or native active-shot evaluation',
   'Owned request policy/model not integrated into editor; 19 legacy leading-slash validator rejections still require separate correction',
   'Full DataModel copy/reference lifetime, per-draw state restoration and shaders remain incomplete'])
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n')
 (OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n')
 print(json.dumps({k:result[k] for k in ['status','cases','exact_output_bytes','successful_loads','file_requests','missing_file_requests','distinct_resources','allocations']},indent=2),flush=True)

if __name__=='__main__':main()
