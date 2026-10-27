"""Actual FindMaterial slot77 cache-hit branch plus benchmark name/alias audit.

Miss/loading and preload rejection are deliberately unexecuted, not invented.
"""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,re,struct,subprocess,tempfile,time
from verify_ifm_draw_packet import check_ranges
from verify_builtin_error_material_seed import source_closure
ROOT=Path(__file__).resolve().parent.parent;OUT=ROOT/'outputs/material-find-cached-20261027'
PARENT=ROOT/'outputs/material-backup-factory-20261027/verification-6zeba3t4/results.json'
LIB=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/materialsystem.dll'
SESSION=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\elements\sessions\mtt_soldier\mtt_soldier.dmx')
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
NEW=['material_find_cache_key_candidate.h','probe_material_find_cached.cpp','build_material_find_cached.cmd','verify_material_find_cached.py']
def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def need(b,s):
 if not b:raise RuntimeError(s)
def key(s):
 s=s.split(b'\0')[0].lower();start=len(s)-1
 while start>0:
  if s[start] in b'./\\':break
  start-=1
 if start>0 and s[start:start+1]==b'.':s=s[:start]
 return s.replace(b'\\',b'/')
def corpus(benchmark):
 values=[b'',b'.',b'..',b'.Hidden',b'dir/.Hidden',b'dir/..',b'a.',b'A/B',b'A\\B.VMT',b'A/B.PNG',b'A/B.Foo.Bar',b'Dir.Name/Leaf',b'/ABSOLUTE/NAME.VMT',b'C:\\PATH\\FILE.VMT',b'../Relative.VMT',b'foo//BAR.vmt',b'foo/./bar.VMT',b'{Procedural}.VMT',b'INPUT.VMT\0IGNORED',bytes(range(128,256))+b'.VMT',b'A'*4095]
 for base in [b'fixture/a',b'fixture/path.with.dot/name',b'fixture/.file',b'fixture/name.']:
  for ext in [b'',b'.vmt',b'.VMT',b'.png',b'.x.y']:
   for upper in [False,True]:
    for backslash in [False,True]:
     s=(base+ext).upper() if upper else base+ext
     values.append(s.replace(b'/',b'\\') if backslash else s)
 for name in benchmark:
  raw=name.encode('utf-8');values.extend([raw,raw.upper(),raw.replace(b'/',b'\\')+b'.VMT'])
 return [(s,key(s),option) for s in values for option in range(8)]
def main():
 OUT.mkdir(exist_ok=True);need(sha(PARENT)=='ad1cb6dafa974f7e85a6a3e380fb4ce78bdcf29b9e92e9c2896356499a3d610d','accepted parent receipt');parent=json.loads(PARENT.read_text());protected=parent['protected']|parent['source'];need(all(sha(p)==h for p,h in protected.items()),'preceding sources/inputs unchanged before');need(sha(LIB)=='c78e414891b4d87e3922610a8bf937bca3efbaab95bced488522045998bc87f8','historical material image');need(sha(SESSION)=='5a08cf8eeb086586f05dc856fb5469dd238fde90315ecf17c2dd4ab5f0742aee','benchmark identity')
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 image,static=check_ranges(LIB,'materialsystem-disassembly.txt',[('whole_FindMaterial77',0x10016f60,0x100172a0),('primary_find',0x10010480,0x10010576),('extension_strip',0x1006e7e0,0x1006e871),('slash_fix',0x1006e880,0x1006e8a6),('lower_wrapper',0x1006e380,0x1006e389),('default_locale_lower',0x10092e76,0x10092ed3),('forwarding_identity',0x100067b0,0x100067b7),('symbol_intern',0x100738e0,0x10073926)])
 need(image.word(0x100984cc+77*4)==0x10016f60 and image.word(0x10095d74+94*4)==0x100067b0,'actual interface virtual identities');(run/'static-reference.json').write_text(json.dumps(static,indent=2)+'\n');sources=source_closure([Path(p) for p in parent['source']]+[ROOT/'work'/p for p in NEW]);started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_find_cached.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180);(run/'build.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'fresh build failed')
 exes=[]
 for tag,machine in [('reference',0x14c),('owned',0x8664)]:
  exe=OUT/('find_cached_'+tag+('_x86.exe' if tag=='reference' else '_x64.exe'));raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];need(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'fresh architecture/manifest');need(not re.search(r'\b(?:warning|error)\b',(OUT/(tag+'-build.log')).read_text(),re.I),'clean build '+tag);imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(tag+'-imports.txt')).write_text(imports);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);need(modules==['KERNEL32.dll'],'system-only imports');exes.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),imports=modules))
 benchmark=run/'benchmark-material-names.tsv';p=subprocess.run([exes[1]['path'],'--benchmark',str(SESSION),str(benchmark)],cwd=run,capture_output=True,text=True,timeout=90);(run/'benchmark.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'benchmark read '+p.stderr);materials={};owners={};rejections=[]
 for line in benchmark.read_text(encoding='utf-8').splitlines():
  v=line.split('\t')
  if v[0]=='M':need(len(v)==4,'material output');materials[int(v[1])]=dict(name=v[2],key=v[3]);need(key(v[2].encode('utf-8')).decode('utf-8')==v[3],'independent benchmark key')
  elif v[0]=='R':need(len(v)==3,'legacy rejection output');rejections.append(dict(material=int(v[1]),error=v[2]))
  else:need(v[0]=='O' and len(v)==4,'owner output');owners.setdefault(int(v[1]),[]).append(dict(position=int(v[2]),material=int(v[3])))
 need(materials and owners and all(row['material'] in materials for rows in owners.values() for row in rows),'benchmark references accounted');groups={};aliases=[]
 for id,m in materials.items():groups.setdefault(m['key'],[]).append(id)
 for model,rows in owners.items():
  seen={}
  for row in rows:seen.setdefault(materials[row['material']]['key'],[]).append(row)
  for identity,rows in seen.items():
   if len(rows)>1:aliases.append(dict(model=model,key=identity,positions=rows))
 referenced={row['material'] for rows in owners.values() for row in rows};need(all(materials[r['material']]['name'].startswith('/') for r in rejections),'known legacy leading-slash rejection cause');audit=dict(scope='raw DMX graph material names/owner arrays; not loaded native chain or active-shot timing',materials=materials,owners=owners,name_groups=groups,within_model_aliases=aliases,referenced_materials=sorted(referenced),unowned_materials=sorted(set(materials)-referenced),legacy_rejections=rejections);(run/'benchmark-audit.json').write_text(json.dumps(audit,indent=2)+'\n')
 cases=corpus([v['name'] for v in materials.values()]);query=run/'query.bin';q=bytearray(struct.pack('<I',len(cases)));expected=bytearray(struct.pack('<I',len(cases)))
 for name,normalized,option in cases:q+=struct.pack('<I',len(name))+name+struct.pack('<I',len(normalized))+normalized+struct.pack('<I',option);expected+=struct.pack('<I',len(normalized))+normalized+struct.pack('<II',2,123)
 query.write_bytes(q);(run/'expected.bin').write_bytes(expected)
 for tag,exe in zip(['reference','owned'],exes):
  target=run/(tag+'.bin');p=subprocess.run([exe['path'],str(LIB if tag=='reference' else run/'absent.dll'),str(query),str(target)],cwd=run,capture_output=True,text=True,timeout=90);(run/(tag+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,tag+' exit '+str(p.returncode)+' '+p.stderr);actual=target.read_bytes()
  if actual!=expected:
   at=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)));raise RuntimeError(tag+' mismatch byte '+str(at))
 native=(run/'reference.log').read_text();m=re.search(r'allocations=(\d+) releases=(\d+)',native);need(m and m[1]==m[2],'native complete heap cleanup')
 safety=subprocess.run([exes[1]['path'],'--self-test'],cwd=run,capture_output=True,text=True,timeout=30);(run/'safety.log').write_text(safety.stdout+safety.stderr);need(safety.returncode==0 and 'checks=5' in safety.stdout,'five owned safety checks')
 need(all(sha(p)==h for p,h in protected.items()) and all(sha(p)==h for p,h in sources.items()),'protected source/input identities unchanged after');need(sha(SESSION)=='5a08cf8eeb086586f05dc856fb5469dd238fde90315ecf17c2dd4ab5f0742aee','benchmark read-only');need(sha(parent['editor'])==parent['editor_sha256'] and all(sha(ROOT/p)==h for p,h in parent['editor_dependencies'].items()),'editor312deps unchanged')
 result=dict(status='VERIFIED cached FindMaterial identity/key branch; PARTIALLY VERIFIED full material lookup',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',cases=len(cases),exact_output_bytes=len(expected),allocations=int(m[1]),releases=int(m[2]),safety=safety.stdout.strip(),benchmark_materials=len(materials),benchmark_models=len(owners),benchmark_owner_positions=sum(map(len,owners.values())),within_model_alias_groups=len(aliases),legacy_rejected_materials=len(rejections),benchmark_unique_name_keys=len(groups),benchmark_shared_name_groups=sum(len(v)>1 for v in groups.values()),benchmark_audit=audit,benchmark_sha256=sha(SESSION),benchmark_audit_sha256=sha(run/'benchmark-audit.json'),executables=exes,parent=str(PARENT),parent_sha256=sha(PARENT),source=sources,protected=protected,static_reference_sha256=sha(run/'static-reference.json'),query_sha256=sha(query),output_sha256=sha(run/'reference.bin'),editor=parent['editor'],editor_sha256=parent['editor_sha256'],editor_dependencies=parent['editor_dependencies'],editor_rebuilt=False,limits=['whole actual FindMaterial77 cached-hit path executes unchanged with original symbol table/tree/normalization/forwarding method','dictionary records are controlled cached raw layouts, not normally constructed/loaded materials; category0 hit always supplied','miss/loading, preload rejection, real material initialization, locale-dependent lowercase and renderer remain unverified','explicit bounded heap/lock/TLS/CRT services; no DllMain/original import loading or live SFM','raw benchmark graph only; no active-shot/native-loaded-chain/visual claim','owned key helper not integrated; current strict session-material validation intentionally unchanged pending scope correction'])
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','exact_output_bytes','allocations','benchmark_materials','benchmark_models','benchmark_owner_positions','within_model_alias_groups']},indent=2),flush=True)
if __name__=='__main__':main()
