"""Native BSP material surface table policy; effective materials remain supplied.
This does not treat raw VMT fields as shader-resolved IMaterial parameters.
"""
import collections,hashlib,io,json,random,re,shutil,struct,subprocess,tempfile,zipfile
from pathlib import Path
from verify_ifm_draw_packet import check_ranges
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/world-surface-table-20261017'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
PRIOR=ROOT/'outputs/world-plane-tags-20261016/verification-0h09c7pq/results.json'
DUMP=Path('C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/dumpbin.exe')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def require(x,message):
 if not x:raise RuntimeError(message)
def lump(b,id):return struct.unpack_from('<II',b,8+16*id)
def names(b):
 td,n=lump(b,2);st,_=lump(b,44);sd,_=lump(b,43);out=[]
 for i in range(n//32):
  index=struct.unpack_from('<I',b,td+32*i+12)[0];offset=struct.unpack_from('<I',b,st+4*index)[0];p=sd+offset;out.append(b[p:b.index(0,p)].decode('ascii'))
 return out
def string(s):
 raw=s.encode('ascii');return struct.pack('<I',len(raw))+raw
def fixture(scripts,params):
 raw=struct.pack('<I',len(scripts))+b''.join(string(n)+string(t) for n,t in scripts)+struct.pack('<I',len(params))
 for name,available,error,primary,secondary in params:
  flags=int(available)|int(error)*2|int(primary is not None)*4|int(secondary is not None)*8;raw+=string(name)+struct.pack('<I',flags)
  for value in [primary,secondary]:
   if value is not None:raw+=string(value)
 return raw
def decode(raw):
 at=0
 def scalar(fmt):
  nonlocal at
  result=struct.unpack_from(fmt,raw,at);at+=struct.calcsize(fmt);return result
 entries=[]
 for _ in range(scalar('<I')[0]):
  n=scalar('<I')[0];name=raw[at:at+n].decode('ascii');at+=n;entries.append((name,*scalar('<2H')))
 mapping=[scalar('<I')[0] for _ in range(scalar('<I')[0])];pairs=[scalar('<2H') for _ in range(scalar('<I')[0])];require(at==len(raw),'Result wire span');return entries,mapping,pairs

def main():
 prior=json.loads(PRIOR.read_text());require(prior['status']=='PASS','Prior milestone');originals=dict(prior['original_hashes'])
 for p,s in originals.items():require(sha(Path(p))==s,'Original changed '+p)
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True);libraries=run/'copied-libraries';libraries.mkdir()
 for n in ['tier0.dll','vphysics.dll']:
  src=GAME/'bin'/n;dst=libraries/n;shutil.copyfile(src,dst);require(sha(src)==sha(dst),'Copied DLL identity')
 image,evidence=check_ranges(GAME/'bin/engine.dll','engine-disassembly.txt',[('texture_material_loop',0x1014ab60,0x1014ac2d),('texinfo_flag_union',0x1014ac60,0x1014acab),('terrain_material_properties',0x1014c740,0x1014c81d),('brush_texinfo_to_material_id',0x1014b791,0x1014b7aa)])
 require(image.string(0x103425a8)=='World textures' and image.string(0x10350934)=='$surfaceprop' and image.string(0x10350c4c)=='$surfaceprop2','Native parameter names');(run/'original-code-evidence.json').write_text(json.dumps(evidence,indent=2));executables=[];builds=[]
 for name in ['table_native_x86','table_owned_x64','table_safety_x64','surface_safety_x64','world_regression_x64','controlled_regression_x64']:
  exe=OUT/(name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];machine=0x14c if name.endswith('x86') else 0x8664;require(struct.unpack_from('<H',binary,pe+4)[0]==machine and b'asInvoker' in binary,'Executable architecture/manifest');modules=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(name+'-imports.txt')).write_text(modules);imports=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',modules,re.M|re.I);require(imports and set(x.lower() for x in imports)=={'kernel32.dll'},'Unexpected dependency');executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=imports));log=OUT/(name+'-build.log');require(not re.search(r'\b(?:warning|error) [A-Z]+\d+',log.read_text()),'Compiler diagnostic '+str(log));builds.append(dict(path=str(log),sha256=sha(log),warnings=0,errors=0))
 def execute(name,args,folder):
  r=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],cwd=folder,capture_output=True,text=True,timeout=120);(folder/(name+'.log')).write_text(r.stdout+r.stderr);require(r.returncode==0,'Probe failed '+str(folder)+' '+name+' exit='+str(r.returncode)+' '+r.stdout+r.stderr);print(name+': '+r.stdout.strip(),flush=True);return r.stdout.strip()
 bsp=GAME/'tf_movies/maps/mtt_soldier.bsp';b=bsp.read_bytes();materialNames=names(b);scripts=[(n,(GAME/where/n).read_text()) for where,n in [('hl2','scripts/surfaceproperties.txt'),('hl2','scripts/surfaceproperties_hl2.txt'),('tf','scripts/surfaceproperties_tf.txt')]];rng=random.Random(20261017);scriptNames=[f'fixture{i}' for i in range(320)];rng.shuffle(scriptNames);large=[('large','default{} '+''.join(n+'{}' for n in scriptNames))];scenarios=[]
 for shift in range(8):
  params=[]
  for i,name in enumerate(materialNames):
   state=(i+shift)%8;params.append((name,state!=0,state==1,None if state in [0,1,2,7] else '' if state==4 else 'missing-unresolved' if state==5 else 'metal', 'wood' if state in [6,7] else None))
  scenarios.append(('mixed-'+str(shift),bsp,scripts,params))
 # Real BSP repeats material IDs; change two of those texinfo flags so original
 # OR aggregation must differ from taking either individual row's flags.
 altered=bytearray(b);ti,tn=lump(b,6);groups=collections.defaultdict(list)
 for i in range(tn//72):groups[struct.unpack_from('<I',b,ti+72*i+68)[0]].append(i)
 shared,indices=next((k,v) for k,v in groups.items() if len(v)>1)
 for row,flag in zip(indices[:2],[0x40000001,0x80000002]):struct.pack_into('<I',altered,ti+72*row+64,flag)
 changed=run/'shared-flags.bsp';changed.write_bytes(altered);params=[(name,True,False,'metal','wood') for name in materialNames];scenarios.append(('shared-flags',changed,scripts,params))
 params=[(name,True,False,scriptNames[(i*17)%320],scriptNames[(i*29+131)%320]) for i,name in enumerate(materialNames)];scenarios.append(('large-property-indices',bsp,large,params));records=[];coverage=collections.Counter()
 for name,map_path,scr,params in scenarios:
  folder=run/name;folder.mkdir();inp=folder/'input.bin';inp.write_bytes(fixture(scr,params));a=folder/'native.bin';o=folder/'owned.bin';nativeLog=execute('table_native_x86',[map_path,inp,a,GAME/'bin/engine.dll',libraries],folder);execute('table_owned_x64',[map_path,inp,o,GAME/'bin/engine.dll',libraries],folder);require(a.read_bytes()==o.read_bytes(),'Native/owned table differs '+name);entries,mapping,pairs=decode(o.read_bytes());require([x[0] for x in entries]==materialNames and len(mapping)==2478 and len(pairs)==43,'Actual BSP binding counts')
  if name=='shared-flags':require(entries[shared][2]&3==3,'Shared flag union not retained')
  if name=='large-property-indices':require(any(x[1]>255 and x[1]!=65535 for x in entries),'Large property domain untested')
  for (_,available,error,primary,secondary),(_,prop,_) in zip(params,entries):
   if not available or error:require(prop==0,'Unavailable/error material must retain zero');coverage['unavailable' if not available else 'error']+=1
   elif primary is None:require(prop==0,'Absent primary must retain zero');coverage['absent_primary']+=1
   elif primary in ['', 'missing-unresolved']:require(prop==65535,'Unknown explicit name must retain FFFF');coverage['empty_or_unknown']+=1
   else:coverage['resolved']+=1
  records.append(dict(name=name,path=str(folder),materials=len(entries),texinfo=len(mapping),displacements=len(pairs),fixture_sha256=sha(inp),map_sha256=sha(map_path),output_sha256=sha(o),native_log=nativeLog,distinct_primary_ids=sorted(set(x[1] for x in entries)),distinct_pairs=sorted(set(pairs))))
 safety={}
 for name in ['table_safety_x64','surface_safety_x64']:safety[name]=execute(name,[bsp],run)
 regressions=[]
 for item in prior['world_cases']:
  folder=run/('controlled-regression' if item['controlled'] else 'world-regression');folder.mkdir();old=Path(item['path']);fixtureRoot=Path('outputs/world-displacement-surface-20261016/verification-3jl6_ipw');source=ROOT/fixtureRoot/('controlled-folded' if item['controlled'] else '');map_path=source/'world.bsp' if item['controlled'] else bsp;mesh=source/'meshes.bin' if item['controlled'] else source/'prepared.bin';query=source/'queries.bin';target=folder/'owned.bin';execute('controlled_regression_x64' if item['controlled'] else 'world_regression_x64',[map_path,mesh,query,target],folder)
  for suffix,expected in [('',item['numeric_sha256']),('.plane-tags',item['tag_sha256']),('.displacement-surface',item['displacement_sha256'])]:require(sha(Path(str(target)+suffix))==expected,'Prior world result changed '+suffix)
  regressions.append(dict(controlled=item['controlled'],queries=item['queries'],path=str(folder),numeric_sha256=sha(target),plane_tags_sha256=sha(Path(str(target)+'.plane-tags'))))
 # Read-only inventory proves why a raw loose-VMT reader cannot yet supply all
 # effective parameters: one actual referenced VMT is a packed patch.
 mounts=[Path(x) for x in (ROOT/'outputs/loose-collision-scene-20261016/verification-ub8mvw0l/mounts.txt').read_text().splitlines() if x];missing=[]
 for name in materialNames:
  if not any((m/'materials'/(name+'.vmt')).is_file() for m in mounts):missing.append(name)
 offset,length=lump(b,40);archive=zipfile.ZipFile(io.BytesIO(b[offset:offset+length]));packed={}
 for name in missing:
  path='materials/'+name+'.vmt';entry=next((x for x in archive.infolist() if x.filename.lower()==path.lower()),None);require(entry is not None,'Referenced material absent from both loose and packed sources');packed[name]=dict(path=entry.filename,compression=entry.compress_type,bytes=entry.file_size,source=archive.read(entry).decode('ascii'))
 for p,s in originals.items():require(sha(Path(p))==s,'Original changed after test '+p)
 changedNames=['source_bsp_trace_surface_flags_candidate.h','source_bsp_collision_surface_table_candidate.h','probe_bsp_collision_surface_table.cpp','test_bsp_collision_surface_table.cpp','build_bsp_collision_surface_table.cmd','verify_bsp_collision_surface_table.py'];changed={str(ROOT/'work'/n):sha(ROOT/'work'/n) for n in changedNames};sources=dict(changed);pending=[ROOT/'work'/n for n in changedNames]+[ROOT/'work/probe_swept_world_trace.cpp',ROOT/'work/test_world_trace_surface.cpp'];seen=set()
 while pending:
  path=pending.pop()
  if path in seen:continue
  seen.add(path);sources[str(path)]=sha(path)
  if path.suffix in ['.cpp','.h']:
   for name in re.findall(r'^\s*#include\s*"([^"]+)"',path.read_text(),re.M):
    child=path.parent/name
    if child.is_file():pending.append(child)
 limits=['Effective material snapshots supplied explicitly, not loaded from raw VMTs. Original native IMaterialSystem/IMaterial/IMaterialVar interfaces are controlled services; actual normal initialized original physics surface lookup executes. No full material/shader/startup equivalence claim.', 'Original texture/terrain policy loops run as unchanged RX fragments plus appended RET/explicit frames, and original texinfo flag function runs in private engine PE; no original loader allocation/startup execution.', 'BSP texdata order, actual87 names/2478 texinfo mappings/43 displacement associations and shared flag OR are reconstructed. General malformed references are checked/rejected rather than emulating unsafe reads.', 'Actual benchmark has one referenced VMT in BSP pakfile as an include/replace patch. Packed/patch precedence, effective shader defaults and production material resolution remain required before these snapshots can be populated automatically.', 'No lighting guard removal, editor integration/build, render/animation/save/serialization changes or original writes; full combined world+prop reference still pending.']
 result=dict(status='PASS',path=str(run),scenarios=records,coverage=dict(coverage),safety=safety,world_regressions=regressions,native_owned_differences=0,packed_material_gap=packed,executables=executables,build_logs=builds,changed_sources=changed,sources=sources,original_hashes=originals,reference_receipts={str(PRIOR):sha(PRIOR)},limitations=limits);receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2));(OUT/'source-integrity.json').write_text(json.dumps(dict(status='PASS',authoritative_receipt=str(receipt),receipt_sha256=sha(receipt),sources=sources,changed_sources=changed,executables=executables,build_logs=builds,original_hashes=originals),indent=2));lines=['VERIFIED material-indexed BSP collision surface table for supplied effective material parameters.','Whole application PARTIALLY VERIFIED.',str(receipt),'Scenarios='+str(len(records))+' materials='+str(sum(x['materials'] for x in records))+' mappings='+str(sum(x['texinfo'] for x in records))+' terrain pairs='+str(sum(x['displacements'] for x in records)),str(dict(coverage)),*safety.values(),'Prior world queries unchanged='+str(sum(x['queries'] for x in regressions)),'Original hashes unchanged='+str(len(originals)),'','Exact freshly built/tested executables:']+[x['path']+' SHA256='+x['sha256'] for x in executables]+['','Changed sources:']+list(changed)+['','Limits:']+limits+['','Reproduce:','cmd /c work\\build_bsp_collision_surface_table.cmd','C:\\Users\\wigwo\\AppData\\Local\\Python\\bin\\python3.14.exe work\\verify_bsp_collision_surface_table.py'];(OUT/'verification.txt').write_text('\n'.join(lines)+'\n');print('PASS '+str(receipt),flush=True)
if __name__=='__main__':main()
