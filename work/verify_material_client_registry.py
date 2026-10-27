"""Authenticate and execute the pinned original client proxy registrations.

Reference evidence only: do not turn a missing registration into a fake proxy.
No original process, DllMain, imported engine service, or original write.
"""
import hashlib,json,re,struct,subprocess,tempfile,time
from pathlib import Path
from datetime import datetime,timezone
from pe_image import Image
from verify_ifm_draw_packet import check_ranges
from verify_builtin_error_material_seed import source_closure
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/material-invis-proxy-20261027'
CLIENT=ROOT/'outputs/material-proxy-evaluation-20261027/copied-bin/client.dll'
DISASM=ROOT/'outputs/material-proxy-evaluation-20261027/client-disassembly-private.txt'
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
NEW=['probe_material_client_registry.cpp','build_material_client_registry.cmd','verify_material_client_registry.py']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(ok,message):
 if not ok:raise RuntimeError(message)
def discover(image):
 entries=[];calls=[]
 # Cover every instruction-aligned direct caller, not only one short pattern.
 for line in DISASM.open():
  m=re.match(r'\s*([0-9A-F]{8}):\s+E8 (?:[0-9A-F]{2} ){4}\s*call\s+10136370\s*$',line)
  if m:calls.append(int(m[1],16))
 need(len(calls)==75 and 0x1009a103 in calls,'74 registration callers plus client provider accessor')
 for call in calls:
  if call==0x1009a103:continue
  off=image.offset(call);raw=image.b[off:off+25]
  if call==0x10492d54:
   start=0x10492d50;end=0x10492d77;factory=image.word(0x10492d5d);name_address=image.word(0x10492d62)
  else:
   need(raw[5:8]==bytes.fromhex('8b1068') and raw[12:18]==bytes.fromhex('8bc88b420868') and raw[22:25]==bytes.fromhex('ffd0c3'),'authenticated ordinary registration shape')
   start=call;end=call+25;factory=image.word(call+8);name_address=image.word(call+18)
  refs=[]
  for rva,size,offset in image.sections:
   data=image.b[offset:offset+size]
   refs.extend(image.base+rva+m.start() for m in re.finditer(re.escape(struct.pack('<I',start)),data))
  need(len(refs)==1 and 0x104ae300<=refs[0]<0x104b1000 and refs[0]%4==0,'unique ordered static initializer pointer')
  entries.append(dict(registration=start,end_exclusive=end,factory=factory,name_address=name_address,name=image.string(name_address),initializer_pointer=refs[0]))
 entries.sort(key=lambda e:e['initializer_pointer'])
 need(len(entries)==74 and len({e['name'].lower() for e in entries})==74,'distinct complete discovered static registry')
 return entries,calls
def main():
 pre=json.loads((OUT/'preflight.json').read_text())
 need(all(sha(p)==h for p,h in pre['protected'].items()),'all1154 protected identities before')
 need(sha(pre['parent'])==pre['parent_sha256']=='1360d3a7af5110746bae975da402105ee9cfa0b631cae6b1418c42736a229bc2','accepted preceding ModelGlowColor receipt')
 parent=json.loads(Path(pre['parent']).read_text());sources=source_closure([Path(p) for p in parent['source']]+[ROOT/'work'/n for n in NEW])
 need(sha(CLIENT)=='91dd1062227b095f1338312aa7268121894970c3e0a5b0d314e42c77f2aba73b','pinned private original client')
 image=Image(CLIENT);entries,calls=discover(image)
 ranges=[('provider_Create_wrapper',0x1009a100,0x1009a111),('Create',0x10136290,0x101362c5),('destructor',0x101362d0,0x10136304),('Register',0x10136310,0x1013636f),('singleton',0x10136370,0x101363c9),('symbol_Find',0x103cc950,0x103cc9a5),('symbol_constructor',0x103ccba0,0x103ccc0b)]
 ranges += [('registration_'+e['name'],e['registration'],e['end_exclusive']) for e in entries]
 _,static=check_ranges(CLIENT,'../outputs/material-proxy-evaluation-20261027/client-disassembly-private.txt',ranges)
 need([image.word(0x104ffe6c+4*i) for i in range(3)]==[0x10136290,0x101362d0,0x10136310],'actual singleton vtable')
 names={e['name'] for e in entries}
 need({'spy_invis','vm_invis','weapon_invis','ParticleSphereProxy','Equals','Multiply','ModelGlowColor','YellowLevel'}<=names and 'invis' not in names and 'AnimatedWeaponSheen' not in names,'distinct actual registration names')
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 (run/'static-reference.json').write_text(json.dumps(static,indent=2)+'\n');(run/'registrations.json').write_text(json.dumps(entries,indent=2)+'\n')
 query=run/'registrations.tsv';query.write_text(''.join(f"{e['registration']:x}\t{e['factory']:x}\t{e['name']}\n" for e in entries))
 started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_client_registry.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180);(run/'build-driver.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'fresh reference build')
 exe=OUT/'client_registry_reference_x86.exe';b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',b,pe+4)[0]==0x14c and struct.unpack_from('<H',b,pe+24)[0]==0x10b and b'asInvoker' in b,'fresh x86 reference PE32 asInvoker')
 log=(OUT/'registry-build.log').read_text();(run/'registry-build.log').write_text(log);need(not re.search(r'\b(?:warning|error) C\d+',log),'clean reference build')
 inspection=subprocess.run([str(DUMP),'/headers','/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/'registry-inspection.txt').write_text(inspection);imports=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',inspection,re.I|re.M);need(imports==['KERNEL32.dll'],'only Windows imports in reference executable')
 p=subprocess.run([str(exe),str(CLIENT),str(query)],cwd=run,capture_output=True,text=True,timeout=120);(run/'registry-run.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'native registry execution exit='+str(p.returncode)+' '+p.stdout+p.stderr)
 registrations=[line.split('\t') for line in p.stdout.splitlines() if line.startswith('REGISTER\t')];need(registrations==[['REGISTER',e['name'],f"{e['factory']:x}"] for e in entries],'every native callback and exact stored factory')
 m=re.search(r'PASS registered=(\d+) checks=(\d+) allocations=(\d+) releases=(\d+) reallocations=(\d+)',p.stdout);need(m and int(m[1])==74 and int(m[2])>=330 and int(m[3])==int(m[4]) and int(m[5])>0,'native lookup/creation/cleanup coverage')
 need('MISSING\tinvis\tDIRECT_AND_PROVIDER_NULL' in p.stdout and 'MISSING\tAnimatedWeaponSheen\tDIRECT_AND_PROVIDER_NULL' in p.stdout,'real exact requested names absent via both entry points')
 # Exact accepted owned regression, no change to application admission policy.
 old=parent['executables']['owned'];need(sha(old['path'])==old['sha256']=='6441e09277d1528cb7462d31c7d94e609b73d74607a5fa0ba51be408db343410','accepted owned x64 safety executable')
 prior=subprocess.run([old['path']],cwd=run,capture_output=True,text=True,timeout=120);(run/'owned-regression.log').write_text(prior.stdout+prior.stderr);need(prior.returncode==0 and 'checks=48 actual_allocation_failures=14' in prior.stdout,'owned material gate remains unchanged')
 need(all(sha(p)==h for p,h in sources.items()) and all(sha(p)==h for p,h in pre['protected'].items()),'all current and protected sources/inputs unchanged')
 need(sha(pre['editor'])==pre['editor_sha256'] and all(sha(ROOT/p)==h for p,h in pre['editor_dependencies'].items()),'editor and313 dependencies unchanged')
 limits=['Independent native x64 reconstruction remains INCOMPLETE. New executable is explicitly x86 reference evidence, not the application or an x64 port',
 'Replayed74 static registration callbacks of one pinned client.dll in initializer-pointer order, with native Register/symbol table/Find/Create/provider wrapper/destructor. All75 direct instruction-aligned singleton callers inspected;73 short callbacks plus ParticleSphereProxy longer callback and one provider accessor',
 'Singleton data fixture reproduces its initialization and runs native symbol constructor. CRT atexit registration deliberately skipped and destructor invoked explicitly. Host bounded tracked alloc/realloc/free supplied. No DllMain, import resolution, SFM process or original installation write',
 'Proves invis and AnimatedWeaponSheen have no factory in this reconstructed static registry; does not prove absence in arbitrary other versions, runtime extensions or original live application. No automatic alias to spy_invis/vm_invis/weapon_invis is justified',
 'Known Equals/Multiply/YellowLevel/ModelGlowColor constructors exercised through direct native Create and actual provider wrapper, and released. Other registered factory pointers verified without executing their unmeasured constructors or Init/Bind',
 'Existing owned store deliberately refuses incomplete proxy lists. This investigation does not alter that policy, implement a fake no-op, admit full weapon materials or reproduce GPU/shader results. Native material-caller skip policy requires a separately verified opt-in integration',
 'Owned ModelGlowColor safety regression reused by exact accepted executable hash, not rebuilt. No new owned runtime code or editor dependency changed; complete editor not rebuilt. Source includes original-address test references only, no embedded original instructions']
 result=dict(status='VERIFIED bounded native client static registry; full application INCOMPLETE',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',registrations=entries,direct_singleton_callers=[hex(x) for x in calls],checks=int(m[2]),allocations=int(m[3]),releases=int(m[4]),reallocations=int(m[5]),native_log=p.stdout,executables={'reference':dict(path=str(exe),sha256=sha(exe),machine='0x14c',imports=imports,fresh=True)},owned_regression=prior.stdout,reused_owned=old,source=sources,protected=pre['protected'],parent=pre['parent'],parent_sha256=pre['parent_sha256'],read_inputs={str(CLIENT):sha(CLIENT),str(DISASM):sha(DISASM)},editor=pre['editor'],editor_sha256=pre['editor_sha256'],editor_dependencies=pre['editor_dependencies'],editor_rebuilt=False,static_sha256=sha(run/'static-reference.json'),registrations_sha256=sha(query),limits=limits)
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','checks','allocations','releases','reallocations','owned_regression']},indent=2),flush=True)
if __name__=='__main__':main()
