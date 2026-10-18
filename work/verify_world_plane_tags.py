"""Original plane import / seeded brush tags / complete point world regressions.
No editor or original installation writes. Native reference scope is recorded.
"""
import collections,hashlib,json,math,random,re,shutil,struct,subprocess,tempfile
from pathlib import Path
from verify_ifm_draw_packet import check_ranges
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/world-plane-tags-20261016'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
PRIOR=ROOT/'outputs/world-displacement-surface-20261016/verification-3jl6_ipw/results.json'
BRUSH=ROOT/'outputs/brush-dispatch-20261015/verification-3au0e9aj/results.json'
DUMP=Path('C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/dumpbin.exe')
NAMES=['plane_native_x86','brush_native_x86','world_native_x86','controlled_native_x86','plane_owned_x64','brush_owned_x64','world_owned_x64','controlled_owned_x64','tag_safety_x64','surface_safety_x64','world_safety_x64']
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
 if not ok:raise RuntimeError(message)
def lump(raw,i):return struct.unpack_from('<II',raw,8+16*i)
def main():
 prior=json.loads(PRIOR.read_text());brushPrior=json.loads(BRUSH.read_text());require(prior['status']=='PASS' and brushPrior['status']=='PASS_NATIVE_GENERAL_OR_OPTIMIZED_BRUSH_DISPATCH','Prior verification status')
 originals=prior['original_hashes']
 for p,s in originals.items():require(sha(Path(p))==s,'Original identity '+p)
 OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True)
 logs=[];executables=[]
 for name in NAMES:
  exe=OUT/(name+'.exe');b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];machine=0x14c if name.endswith('x86') else 0x8664;require(b[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',b,pe+4)[0]==machine and b'asInvoker' in b,'Executable identity '+name)
  imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(name+'-imports.txt')).write_text(imports);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require(modules and set(x.lower() for x in modules)=={'kernel32.dll'},'Unexpected dependency '+name);executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules))
  log=OUT/(name+'-build.log');require(not re.search(r'\b(?:warning|error) [A-Z]+\d+',log.read_text()),'Compiler diagnostic '+str(log));logs.append(dict(path=str(log),sha256=sha(log),warnings=0,errors=0))
 for name in ['native_general','native_box','native_dispatch','native_stationary','owned_general','owned_box','owned_dispatch','owned_stationary']:
  log=OUT/('compat_'+name+'-build.log');require(not re.search(r'\b(?:warning|error) [A-Z]+\d+',log.read_text()),'Compatibility compile diagnostic '+str(log));logs.append(dict(path=str(log),sha256=sha(log),warnings=0,errors=0,scope='compile-only previous probe macro combination'))
 image,evidence=check_ranges(GAME/'bin/engine.dll','engine-disassembly.txt',[
  ('plane_import_loop',0x1014b1f3,0x1014b40b),('plane_import_clear_argument',0x1014b1eb,0x1014b1f3),('hunk_clear_branch',0x101e5339,0x101e534b),
  ('plane_copy_all_four_tags',0x100e9720,0x100e975d),('optimized_box',0x10146850,0x10146c24),('general_brush',0x10147690,0x1014791f),('surface_helper',0x10146440,0x1014646a),('terrain_hit_plane',0x1015b520,0x1015b5a3),('whole_world_wrapper',0x10149eb0,0x1014a12c)])
 (run/'original-code-evidence.json').write_text(json.dumps(evidence,indent=2))
 # Check embedded brush/box bytes and constants against the pinned installation.
 for header,entries in [('native_convex_brush_ray_bytes.h',[('nativeBrushBytes',0x10147690,0x1014791f),('nativePlaneCopyBytes',0x100e9720,0x100e975d),('nativeSurfaceBytes',0x10146440,0x1014646a)]),('native_optimized_box_bytes.h',[('nativeBoxBytes',0x10146850,0x10146c24)])]:
  text=(ROOT/'work'/header).read_text()
  for name,a,z in entries:
   match=re.search(r'\b'+name+r'\[\]=\{([^}]+)\}',text);require(match is not None,'Embedded original array missing');actual=bytes(int(x,16) for x in re.findall('0x([0-9a-fA-F]+)',match[1]));offset=image.offset(a);require(actual==image.b[offset:offset+z-a],'Stale embedded original '+name)
   require(actual in (OUT/'brush_native_x86.exe').read_bytes(),'Original absent from native brush executable')
 text=(ROOT/'work/native_optimized_box_bytes.h').read_text()
 for offset,raw in re.findall(r'\{0x([0-9a-f]+),\{([^}]+)\}\}',text):
  actual=bytes(int(x,16) for x in re.findall('0x([0-9a-f]+)',raw));p=image.offset(0x10000000+int(offset,16));require(len(actual)==16 and actual==image.b[p:p+16],'Stale native box constant')
 def execute(name,args,folder=run):
  log=folder/(name+'.log');r=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],cwd=folder,capture_output=True,text=True,timeout=120);log.write_text(r.stdout+r.stderr);require(r.returncode==0,'Probe failed '+str(log)+' exit='+str(r.returncode)+' '+r.stdout+r.stderr);print(name+': '+r.stdout.strip(),flush=True);return r.stdout.strip()
 # All actual map plane inputs plus unrolled/scalar boundary lengths and all
 # sign combinations, signed zero, subnormals, finite extreme values, types0..5.
 bsp=GAME/'tf_movies/maps/mtt_soldier.bsp';raw=bsp.read_bytes();po,pn=lump(raw,1);rng=random.Random(20261017);values=[0.,-0.,1.,-1.,.5,-.5,struct.unpack('<f',bytes.fromhex('01000000'))[0],struct.unpack('<f',bytes.fromhex('01000080'))[0],3.4028234663852886e38,-3.4028234663852886e38]
 batches=[raw[po:po+pn]];counts=[0,1,2,3,4,5,6,7,8,9,15,16,17,31,32,33,257,4095,4096,4097,65535,65536]
 for n in counts:
  rows=[]
  for i in range(n):
   normal=[values[(i//(10**a))%10] for a in range(3)];distance=values[(i//7)%10];rows.append(struct.pack('<4fI',*normal,distance,i%6))
  batches.append(b''.join(rows))
 inp=run/'plane-input.bin';inp.write_bytes(b''.join(struct.pack('<I',len(x)//20)+x for x in batches));a=run/'planes-native.bin';b=run/'planes-owned.bin';execute('plane_native_x86',[inp,a,GAME/'bin/engine.dll']);execute('plane_owned_x64',[inp,b,GAME/'bin/engine.dll']);pa,pb=a.read_bytes(),b.read_bytes();require(pa==pb and len(pb)==sum(map(len,batches)),'Original/owned plane import differs')
 tags=collections.Counter();cursor=0
 for batch in batches:
  for j in range(0,len(batch),20):
   normal=struct.unpack_from('<3f',batch,j);type=struct.unpack_from('<I',batch,j+16)[0];expected=bytes([type,sum((1<<k) for k in range(3) if normal[k]<0),0,0]);require(pb[cursor:cursor+16]==batch[j:j+16] and pb[cursor+16:cursor+20]==expected,'Imported plane fields or signed-zero semantics');tags[tuple(expected)]+=1;cursor+=20
 require({t[1] for t in tags}==set(range(8)) and {t[0] for t in tags}==set(range(6)),'Plane sign/type coverage');importProof=dict(batches=len(batches),planes=len(pb)//20,actual_map_planes=pn//20,boundary_counts=counts,input_sha256=sha(inp),result_sha256=sha(b),all_sign_combinations=True,types=list(range(6)),native_zero_and_poisoned_destination_guards=True)
 # Original dispatch corpus, plus the same cube with actual axial plane types
 # to select native optimized dispatch. Incoming trace padding differs from
 # synthetic per-plane reserved bytes, proving selective vs whole-plane copy.
 bp=BRUSH.parent;cases=[]
 for dataset,map_path in [(0,bp/'cube.bsp'),(1,bsp),(2,None)]:
  folder=run/('brush-'+str(dataset));folder.mkdir()
  if dataset==2:
   controlled=bytearray((bp/'cube.bsp').read_bytes());offset,_=lump(controlled,1)
   for axis in range(3):
    for sign in range(2):struct.pack_into('<I',controlled,offset+20*(2*axis+sign)+16,axis)
   map_path=folder/'axial-cube.bsp';map_path.write_bytes(controlled)
  source=bp/(str(min(dataset,1) if dataset<2 else 0)+'-queries.bin');query=folder/'queries.bin';shutil.copyfile(source,query);qbytes=query.read_bytes();count=len(qbytes)//48;a=folder/'native.bin';b=folder/'owned.bin';execute('brush_native_x86',[map_path,query,a],folder);execute('brush_owned_x64',[map_path,query,b],folder);require(a.read_bytes()==b.read_bytes() and len(b.read_bytes())==count*36,'Brush numeric regression '+str(dataset))
  ta=Path(str(a)+'.plane-tags').read_bytes();tb=Path(str(b)+'.plane-tags').read_bytes();require(ta==tb and len(tb)==count*4,'Seeded brush plane tags differ '+str(dataset));coverage=collections.Counter(tuple(tb[i:i+4]) for i in range(0,len(tb),4));require((0xa1,0xb2,0xc3,0xd4) in coverage,'Missing retained trace seed')
  if dataset<2:require(sha(b)==brushPrior['datasets'][dataset]['output_sha256'],'Prior dispatch numeric output changed')
  if dataset in [0,2]:
   for axis in range(3):
    for sign in [0,1]:require((axis if dataset==2 else 3,(1<<axis) if sign else 0,0xc3 if dataset==2 else 0x35,0xd4 if dataset==2 else 0x79) in coverage,'Missing all-face/general-plane-copy coverage')
  cases.append(dict(dataset=dataset,map=str(map_path),queries=count,query_sha256=sha(query),numeric_sha256=sha(b),tag_sha256=sha(Path(str(b)+'.plane-tags')),tags={','.join(map(str,k)):v for k,v in coverage.items()}))
 # Fresh original whole-world wrapper and owned world trace on frozen complete
 # world corpora. Legacy76 numeric/surface bytes and prior D identity retained.
 old=PRIOR.parent;require(sha(old/'prepared.bin')==prior['prepared_sha256'] and sha(old/'queries.bin')==prior['queries_sha256'],'Prior world fixture identity');worlds=[]
 for controlled in [False,True]:
  folder=run/('controlled-world' if controlled else 'benchmark-world');folder.mkdir();src=Path(prior['controlled']['path']) if controlled else old;map_path=src/'world.bsp' if controlled else bsp;mesh=src/'meshes.bin' if controlled else old/'prepared.bin';query=src/'queries.bin';a=folder/'native.bin';b=folder/'owned.bin';prefix='controlled_' if controlled else 'world_';execute(prefix+'native_x86',[map_path,mesh,query,a,GAME/'bin/engine.dll'],folder);execute(prefix+'owned_x64',[map_path,mesh,query,b],folder);n=len(query.read_bytes())//36;require(a.read_bytes()==b.read_bytes() and len(b.read_bytes())==n*76,'World numeric/surface difference');require(b.read_bytes()==(src/'owned.bin').read_bytes(),'Prior world numeric/surface bytes changed');ta=Path(str(a)+'.plane-tags').read_bytes();tb=Path(str(b)+'.plane-tags').read_bytes();require(ta==tb and len(tb)==n*4,'World plane tags differ '+str(controlled));da=Path(str(a)+'.displacement-surface').read_bytes();db=Path(str(b)+'.displacement-surface').read_bytes();require(da==db and db==Path(str(src/'owned.bin')+'.displacement-surface').read_bytes(),'Previous displacement identity changed')
  coverage=collections.Counter();typetags=collections.Counter();output=b.read_bytes();queries=query.read_bytes();witnesses=[]
  for i in range(n):
   tag=tuple(tb[4*i:4*i+4]);typetags[tag]+=1;surface=struct.unpack_from('<I',output,76*i+68)[0];stationary=not any(struct.unpack_from('<3f',queries,i*36+12));coverage['stationary' if stationary else 'swept']+=1
   if surface==0xfffffffe:
    coverage['displacement']+=1;coverage['stationary_displacement']+=stationary;coverage['displacement_retains_nonzero_tags']+=any(tag)
    normal=struct.unpack_from('<3f',output,76*i+20)
    if normal!=(0.,0.,0.) and tag==(0,0,0,0) and len(witnesses)<3:witnesses.append(dict(query=i,normal=normal,tags=tag,stationary=stationary))
  require(coverage['displacement']>0 and witnesses,'Terrain retained-tag coverage')
  if controlled:require(coverage['stationary_displacement']==36,'Controlled stationary terrain coverage changed')
  worlds.append(dict(controlled=controlled,queries=n,path=str(folder),coverage=dict(coverage),terrain_retention_witnesses=witnesses,tags={','.join(map(str,k)):v for k,v in typetags.items()},numeric_sha256=sha(b),tag_sha256=sha(Path(str(b)+'.plane-tags')),displacement_sha256=sha(Path(str(b)+'.displacement-surface'))))
 safety={}
 for name in ['tag_safety_x64','surface_safety_x64','world_safety_x64']:safety[name]=execute(name,[bsp])
 for p,s in originals.items():require(sha(Path(p))==s,'Original changed '+p)
 names=['source_bsp_world_collision_data_candidate.h','source_optimized_box_line_ray_candidate.h','source_brush_line_dispatch_candidate.h','probe_swept_world_trace.cpp','probe_native_convex_brush_ray.cpp','probe_world_plane_import.cpp','test_world_plane_tags.cpp','build_world_plane_tags.cmd','verify_world_plane_tags.py'];changed={str(ROOT/'work'/n):sha(ROOT/'work'/n) for n in names};sources=dict(changed);pending=[ROOT/'work'/n for n in names]+[ROOT/'work/test_world_trace_surface.cpp',ROOT/'work/test_world_ray_trace.cpp'];visited=set()
 while pending:
  p=pending.pop()
  if p in visited:continue
  visited.add(p);sources[str(p)]=sha(p)
  if p.suffix in ['.h','.cpp']:
   for name in re.findall(r'^\s*#include\s*"([^"]+)"',p.read_text(),re.M):
    child=p.parent/name
    if child.is_file():pending.append(child)
 limits=['Native import oracle executes original loop instructions in a relocated RX fragment with appended RET/explicit frame; guards prove local writes, not full original loader/allocator/startup execution. Actual allocation clearing is static pinned instruction evidence.', 'World oracle executes original world wrapper/kernels in private PE with existing controlled work-pool/release/memset hooks and prepared meshes; not live SFM or combined prop scene. Native world plane records are supplied by the separately native-verified checked importer.', 'Seeded per-brush tests deliberately use nonzero plane padding and trace padding. Production loaded plane padding stays zero. Existing36/76 numeric wires unchanged; added4-byte tag sidecars.', 'Brush surface IDs and terrain material-property pairs remain controlled fixtures. Actual VMT/runtime world material bindings and remaining complete world-trace metadata still required.', 'No renderer, animation, serialization/save, editor integration/build, lighting guard removal, original writes, dependencies, SFM/Blender/UI processes or speedup claims.']
 result=dict(status='PASS',date='2026-10-17',path=str(run),import_proof=importProof,brush_cases=cases,world_cases=worlds,safety=safety,plane_tag_differences=0,legacy_numeric_differences=0,prior_displacement_differences=0,executables=executables,build_logs=logs,changed_sources=changed,sources=sources,original_hashes=originals,reference_receipts={str(p):sha(p) for p in [PRIOR,BRUSH]},limitations=limits)
 receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2));(OUT/'source-integrity.json').write_text(json.dumps(dict(status='PASS',authoritative_receipt=str(receipt),receipt_sha256=sha(receipt),sources=sources,changed_sources=changed,executables=executables,build_logs=logs,original_hashes=originals),indent=2));lines=['VERIFIED world collision plane tags within tested point-ray scope.','Whole application PARTIALLY VERIFIED.',str(receipt),'Imported planes='+str(importProof['planes']),'Seeded brush queries='+str(sum(x['queries'] for x in cases)),'World queries='+str(sum(x['queries'] for x in worlds)),'All compared fields exact; legacy numeric/surface/terrain outputs unchanged.',*safety.values(),'Original hashes unchanged='+str(len(originals)),'','Freshly compiled/tested executables:']+[e['path']+' SHA256='+e['sha256'] for e in executables]+['','Changed sources:']+list(changed)+['','Scope:']+limits+['','Reproduce:','cmd /c work\\build_world_plane_tags.cmd','C:\\Users\\wigwo\\AppData\\Local\\Python\\bin\\python3.14.exe work\\verify_world_plane_tags.py'];(OUT/'verification.txt').write_text('\n'.join(lines)+'\n');print('PASS '+str(receipt),flush=True)
if __name__=='__main__':main()
