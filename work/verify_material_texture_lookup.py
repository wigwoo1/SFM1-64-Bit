"""Original borrowed texture wrapper/cache path vs owned x64 service boundary."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,re,struct,subprocess,tempfile,time,random,itertools
from verify_ifm_draw_packet import check_ranges
from verify_builtin_error_material_seed import source_closure
ROOT=Path(__file__).resolve().parent.parent;OUT=ROOT/'outputs/material-texture-lookup-20261027'
LIB=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/materialsystem.dll'
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
NEW=['material_texture_lookup_candidate.h','probe_material_texture_lookup.cpp','build_material_texture_lookup.cmd','verify_material_texture_lookup.py']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b,s):
 if not b:raise RuntimeError(s)
def pack(*words):return struct.pack('<'+'I'*len(words),*(v&0xffffffff for v in words))
def text(s):return pack(len(s))+s
def key(raw):
 s=raw.split(b'\0')[0].lower()
 if not s.endswith(b'.hdr'):
  i=len(s)-1
  while i>0 and s[i:i+1] not in [b'.',b'/',b'\\']:i-=1
  if i>0 and s[i:i+1]==b'.':s=s[:i]
 return s.replace(b'\\',b'/')
def keys(benchmark):
 values=[b'.',b'..',b'.HDR',b'file.HDR',b'FILE.hDr',b'models\\A.HDR',b'file.png',b'file.vtf',b'file.HDR.foo',b'/MODELS/A.VTF',b'foo//BAR.vtf',b'foo/../BAR.vtf',b'foo/./BAR.vtf',b'C:\\PATH\\A.VTF',b'input.HDR\0ignored',b'[ 1.000000 1.000000 1.000000 1.000000 ]',b'default',b'_rt_FullFrameFB1',b'_rt_FullScreen',b' ']
 values += [s.encode('ascii') for s in benchmark if s and len(s.encode('ascii'))<=259]
 for length in [1,4,5,254,255,256,258,259]:values += [b'A'*length,(b'A'*max(0,length-4))+b'.HDR']
 rng=random.Random(2026102759)
 for _ in range(512):
  parts=[''.join(rng.choice('ABcd_09') for _ in range(rng.randint(1,12))) for _ in range(rng.randint(1,4))];values.append(rng.choice(['/','\\']).join(parts).encode()+rng.choice([b'',b'.vtf',b'.HDR',b'.png',b'.x.y']))
 rows=[]
 for value in sorted(set(values)):
  for variant in [value,value.upper(),value.replace(b'/',b'\\')]:
   for mode in [0,1]:rows.append((mode,variant,key(variant)))
 # Direct-cache-only safe misses and manager null shortcut. No full load on miss.
 for value in [b'fixture/missing',b'../missing.vtf',b'/missing.hdr',b'not-an-alias']:
  rows.append((2,value,b'fixture/unrelated'))
 rows += [(3,b'',b'fixture/unrelated'),(4,b'',b'fixture/unrelated')]
 return rows
def key_packets(rows):
 q=pack(len(rows));expected=pack(len(rows))
 for ci,(mode,value,seed) in enumerate(rows):
  q+=pack(mode)+text(value)+text(seed);k=b'' if mode==3 else key(value);found=int(mode<=1 and k==seed)
  expected+=pack(ci)+text(k)+pack(found,found if mode<=1 else 0 if mode==3 else 0xffffffff,found if mode<=1 else 0xffffffff)
 return q,expected
def routes(benchmark):
 values=[b'',b'fixture/valid',b'fixture/missing',b'_rt_FullFrameFB1',b'_RT_FULLFRAMEFB1',b'_rt_fullframefb1',b'_rt_FullScreen',b' _rt_FullFrameFB1',b'_rt_FullFrameFB1 ',b'default',b'[ 1 1 1 1 ]',b'INPUT\0tail',b'_rt_FullFrameFB1\0tail',b'A'*259]
 values += [s.encode('ascii') for s in benchmark if s and len(s.encode('ascii'))<=259]
 return list(itertools.product(sorted(set(values)),[0,1],[0,1],[0,1,2,255],[None,b'',b'Other textures',b'GROUP\0ignored'],[0,1,-1,-2147483648,2147483647]))
def route_packets(rows):
 q=pack(len(rows));expected=pack(len(rows));redirects=warnings=0
 for ci,(raw,first,second,complain,group,flags) in enumerate(rows):
  q+=pack(first,second,complain,int(group is not None),flags)+text(raw)+text(group or b'');name=raw.split(b'\0')[0];g=b'NULL' if group is None else b'G:'+group.split(b'\0')[0]
  trace=b'F1 '+name+b' '+g+b' '+str(flags).encode()+b'\nE1 '+str(first).encode()+b'\n';redirect=first and name.lower()==b'_rt_fullframefb1';result=1;error=first
  if redirect:
   redirects+=1;result=2;error=second;name=b'_rt_FullScreen';trace+=b'F2 '+name+b' '+g+b' '+str(flags).encode()+b'\nE2 '+str(second).encode()+b'\n'
  warning=bool(error and complain)
  if warning:trace+=b'W '+name+b'\n';warnings+=1
  expected+=pack(ci,result,int(warning),int(bool(redirect)))+text(trace)
 return q,expected,redirects,warnings
def main():
 pre=json.loads((OUT/'preflight.json').read_text());need(all(sha(p)==h for p,h in pre['protected'].items()),'1282 protected entries before');need(sha(pre['parent'])==pre['parent_sha256']=='477e8d9ed32b836a84fa2bb990e9916c58cb310a115447bea6a4223630162a73','accepted texture lifetime receipt');parent=json.loads(Path(pre['parent']).read_text());need(sha(LIB)=='c78e414891b4d87e3922610a8bf937bca3efbaab95bced488522045998bc87f8','pinned private material system')
 image,evidence=check_ranges(LIB,'materialsystem-disassembly.txt',[('public_FindTexture83',0x1000b7e0,0x1000b87a),('manager_constructor',0x10041e00,0x10041eea),('texture_cache_key',0x1002c390,0x1002c3f5),('cache_lookup',0x10042080,0x100421ef),('manager_find_or_load',0x10042350,0x100423aa),('cache_compare',0x100367b0,0x100367e0),('alias_tree_find',0x1003e890,0x1003e986)])
 need(image.word(0x100984cc+83*4)==0x1000b7e0 and image.word(0x1009cc74+7*4)==0x10042350 and image.word(0x10101a84)==0x1011b370,'public and texture-manager virtual identities')
 need(image.string(image.word(0x100fc7b8))=='_rt_FullFrameFB1' and image.string(image.word(0x100fc7bc))=='_rt_FullScreen' and image.string(0x1009ab74)=='.hdr','actual exception/alias strings')
 sources=source_closure([Path(p) for p in parent['source']]+[ROOT/'work'/p for p in NEW]);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True);(run/'static-reference.json').write_text(json.dumps(evidence,indent=2)+'\n')
 inventory=Path(parent['parent']).parent/'inventory-audit.json';originalAudit=json.loads(inventory.read_text());need(sha(inventory)==json.loads(Path(parent['parent']).read_text())['audit_sha256'],'accepted observed benchmark strings');names=sorted({r['texture_request'] for r in originalAudit['string_override_rows'] if r['target_exists']})
 started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_texture_lookup.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180);(run/'build-driver.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'fresh original-reference and x64 builds')
 exes={}
 for tag,machine,magic in [('reference',0x14c,0x10b),('owned',0x8664,0x20b)]:
  exe=OUT/('texture_lookup_'+tag+('_x86.exe' if tag=='reference' else '_x64.exe'));b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==magic and b'asInvoker' in b,'fresh architecture/manifest '+tag)
  log=(OUT/(tag+'-build.log')).read_text();(run/(tag+'-build.log')).write_text(log);need(not re.search(r'\b(?:warning|error) C\d+',log),'clean compile '+tag);inspection=subprocess.run([str(DUMP),'/headers','/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(tag+'-inspection.txt')).write_text(inspection);imports=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',inspection,re.I|re.M);need(imports==['KERNEL32.dll'],'Windows-only direct imports '+tag);exes[tag]=dict(path=str(exe),sha256=sha(exe),machine=hex(machine),imports=imports,fresh=True)
 modes={'keys':keys(names),'routes':routes(names)};counts={};logs={}
 for mode,rows in modes.items():
  if mode=='keys':query,expected=key_packets(rows)
  else:query,expected,redirects,warnings=route_packets(rows)
  (run/(mode+'-query.bin')).write_bytes(query);(run/(mode+'-expected.bin')).write_bytes(expected)
  for tag in exes:
   dest=run/(mode+'-'+tag+'.bin');p=subprocess.run([exes[tag]['path'],'--'+mode,str(LIB if tag=='reference' else run/'absent.dll'),str(run/(mode+'-query.bin')),str(dest)],cwd=run,capture_output=True,text=True,timeout=120);(run/(mode+'-'+tag+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,mode+' '+tag+' runtime: '+p.stdout+p.stderr);actual=dest.read_bytes()
   if actual!=expected:
    at=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)));raise RuntimeError(mode+' '+tag+' independent mismatch at byte '+str(at))
   logs[mode+'_'+tag]=p.stdout
  counts[mode]=dict(cases=len(rows),exact_bytes=len(expected),sha256=sha(run/(mode+'-reference.bin')))
 p=subprocess.run([exes['owned']['path']],cwd=run,capture_output=True,text=True,timeout=30);(run/'safety.log').write_text(p.stdout+p.stderr);need(p.returncode==0 and 'texture_lookup_safety=24' in p.stdout,'owned unavailable/throwing/null/sentinel/oversized service, callback request/service replacement, reentry and64bit-token safety')
 regressions={}
 for name,entry in parent['reused_executables'].items():
  need(sha(entry['path'])==entry['sha256'],'accepted '+name+' x64 executable');test=subprocess.run([entry['path']],cwd=run,capture_output=True,text=True,timeout=120);(run/(name+'-regression.log')).write_text(test.stdout+test.stderr);need(test.returncode==0 and test.stdout==parent['regressions'][name],'unchanged '+name+' safety');regressions[name]=test.stdout
 need(all(sha(p)==h for p,h in sources.items()) and all(sha(p)==h for p,h in pre['protected'].items()),'all sources/protected unchanged after');need(sha(pre['editor'])==pre['editor_sha256'] and all(sha(ROOT/p)==h for p,h in pre['editor_dependencies'].items()),'unchanged editor and313 dependencies')
 limits=['Independent x64 application remains INCOMPLETE. New owned header implements opt-in logical cache key/public lookup service boundary with borrowed identities, not texture ownership/cache allocation, VTF loading, renderer or GPU admission',
 'Original material-system public slot83 and actual texture manager Find/FindOrLoad execute unmodified. Cached primary/one-level alias tests use original manager constructor and lookup/comparator over explicit one-node cache DATA; actual cache insertion/removal/allocation or arbitrary alias graphs are not verified',
 'Direct cache-only misses/empty names and manager null-input shortcut are safe tests. Whole public wrapper with null input/result or sentinel would dereference invalid pointers and is NOT executed; owned returns unavailable instead. Whole manager load path on miss and actual texture creation/Download/error-texture construction remain UNVERIFIED',
 'Public wrapper route tests use explicit manager-return/error-query/warning spies. Alias retry/error results proven at this boundary do not prove real engine missing-texture results. Cache-hit path allocates/releases no resources; texture retain/release slots are unexpected and would terminate the probe',
 'Texture names preserve final .hdr suffix, unlike ordinary material-key extension stripping. ASCII name prefixes<=259, group prefixes<=255 and raw strings<=4095 admitted; embedded NUL terminates original request. No filesystem path permission or unsafe native truncation/locale equivalence claim',
 'All original code remains RX unchanged; private pinned checkpoint only, no DllMain/import resolution/live SFM/computer use/original writes. Intentional current ifm.dll/gameinfo.txt changes untouched',
 'Two fresh clean builds: private reference x86 and owned x64. Existing997/479 and606/572 x64 profile/binding executables reused by exact hashes, not rebuilt. Complete editor unchanged and not rebuilt; texture overrides still deliberately refused pending actual retained ownership and lookup-miss implementation']
 result=dict(status='VERIFIED bounded borrowed texture lookup/key and original cached route; full texture/runtime INCOMPLETE',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',counts=counts,redirect_cases=redirects,warning_cases=warnings,benchmark_string_names=len(names),safety_checks=24,safety=p.stdout,logs=logs,executables=exes,regressions=regressions,reused_executables=parent['reused_executables'],source=sources,protected=pre['protected'],parent=pre['parent'],parent_sha256=pre['parent_sha256'],read_inputs={str(LIB):sha(LIB),str(inventory):sha(inventory),str(ROOT/'work/materialsystem-disassembly.txt'):sha(ROOT/'work/materialsystem-disassembly.txt')},static_sha256=sha(run/'static-reference.json'),editor=pre['editor'],editor_sha256=pre['editor_sha256'],editor_dependencies=pre['editor_dependencies'],editor_rebuilt=False,limits=limits)
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','counts','redirect_cases','warning_cases','benchmark_string_names','safety_checks','regressions']},indent=2),flush=True)
if __name__=='__main__':main()
