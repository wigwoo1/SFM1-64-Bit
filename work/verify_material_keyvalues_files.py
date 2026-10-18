"""Original real-filesystem KeyValues load vs owned provider-based resolver.
The owned resolver receives explicit source catalogs, not native outputs.
Mounted/BSP priority is observed, not claimed implemented by the resolver.
"""
import collections,hashlib,io,json,random,re,shutil,struct,subprocess,tempfile,zipfile
from pathlib import Path
from verify_material_keyvalues_parser import text,decode
from verify_ifm_draw_packet import check_ranges
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/material-keyvalues-files-20261017'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
PRIOR=ROOT/'outputs/material-keyvalues-parser-20261017/verification-n2zi61gz/results.json'
DUMP=Path('C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/dumpbin.exe')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def require(ok,message):
 if not ok:raise RuntimeError(message)
def run_exe(exe,args,run,label):
 p=subprocess.run([str(exe),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=120);log=run/(label+'.log');log.write_text(p.stdout+p.stderr);require(p.returncode==0,'Program exit '+str(p.returncode)+'; see '+str(log));require('KeyValues Error:' not in p.stdout+p.stderr and "Couldn't load included" not in p.stdout+p.stderr,'Original diagnostics; see '+str(log));print(next((x for x in reversed(p.stdout.splitlines()) if x.startswith('PASS ')),label+' completed'),flush=True);return p.stdout.strip()
def main():
 prior=json.loads(PRIOR.read_text());require(prior['status']=='PASS','Previous checkpoint');originals=dict(prior['original_hashes']);originals[str(GAME/'bin/filesystem_stdio.dll')]=sha(GAME/'bin/filesystem_stdio.dll')
 for path,digest in originals.items():require(sha(Path(path))==digest,'Original identity '+path)
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True);libs=run/'copied-libraries';libs.mkdir()
 for name in ['tier0.dll','vstdlib.dll','materialsystem.dll','filesystem_stdio.dll']:shutil.copyfile(GAME/'bin'/name,libs/name);require(sha(libs/name)==originals[str(GAME/'bin'/name)],'Copied library identity')
 _,evidence=check_ranges(GAME/'bin/materialsystem.dll','materialsystem-disassembly.txt',[('LoadFromFile',0x10071da0,0x10071ef4),('relative_include_loader',0x10071f00,0x1007209f),('LoadFromBuffer',0x100720a0,0x10072516),('base_root_merge',0x100716c0,0x100717b9),('base_recursive_merge',0x100712a0,0x10071376),('base_case_sensitive_compare',0x1006e2c0,0x1006e300)])
 _,fsEvidence=check_ranges(GAME/'bin/filesystem_stdio.dll','filesystem-disassembly.txt',[('AddSearchPath',0x1000ea40,0x1000eae9)]);(run/'filesystem-code-evidence.json').write_text(json.dumps(fsEvidence,indent=2))
 (run/'original-code-evidence.json').write_text(json.dumps(evidence,indent=2));executables=[]
 for name,machine in [('files_native_x86',0x14c),('files_owned_x64',0x8664)]:
  exe=OUT/(name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine and b'asInvoker' in binary,'Executable architecture/manifest');imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(name+'-imports.txt')).write_text(imports);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require(modules and set(x.lower() for x in modules)=={'kernel32.dll'},'Unexpected direct import');executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules))
 scenarios=[]
 def compare(label,mounts,catalog,cases):
  nativeInput=run/(label+'-native-input.bin');ownedInput=run/(label+'-owned-input.bin');nativeOut=run/(label+'-native.bin');ownedOut=run/(label+'-owned.bin')
  nativeInput.write_bytes(struct.pack('<I',len(mounts))+b''.join(text(str(p))+text(id)+struct.pack('<I',pos) for p,id,pos in mounts)+struct.pack('<I',len(cases))+b''.join(text(name)+text('GAME') for name in cases))
  ownedInput.write_bytes(struct.pack('<I',len(catalog))+b''.join(text(name)+text(source) for name,source in catalog.items())+struct.pack('<I',len(cases))+b''.join(text(name) for name in cases))
  run_exe(OUT/'files_native_x86.exe',[libs,nativeInput,nativeOut],run,label+'-native');run_exe(OUT/'files_owned_x64.exe',[ownedInput,ownedOut],run,label+'-owned');a,b=nativeOut.read_bytes(),ownedOut.read_bytes();native,owned=decode(a),decode(b)
  if a!=b:
   indexes=[i for i,(x,y) in enumerate(zip(native,owned)) if x!=y];first=indexes[0];diff=run/(label+'-difference.json');diff.write_text(json.dumps(dict(case=cases[first],native=native[first],owned=owned[first],indexes=indexes),indent=2));raise RuntimeError('Resolver difference '+str(diff))
  require(all(x['ok']==1 for x in native),'Positive load result');scenarios.append(dict(name=label,cases=len(cases),roots=sum(len(x['roots']) for x in native),native_input_sha256=sha(nativeInput),owned_input_sha256=sha(ownedInput),output_sha256=sha(ownedOut)));return native
 high=run/'high';low=run/'low';high.mkdir();low.mkdir();catalog={}
 def write(name,source):
  p=high/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(source.encode('ascii'));catalog[name.lower()]=source
 fixed={'inc.vmt':'Included{iv 7} IncludedTwo{iv 8}','base.vmt':'Base{same base Same capital nested{y base x base} scalar{child base} dup one dup two new 0x1234567890ABCDEF} Ignored{ignored 1}','base2.vmt':'Base{same other new other z later}','plain.vmt':'S{own local}','include-before.vmt':'#include inc.vmt S{local 1}','include-after.vmt':'S{local 1} #include inc.vmt','include-interleave.vmt':'S{a 1} #include inc.vmt Other{b 2}','base-merge.vmt':'#base base.vmt S{same local nested{x local} scalar 3 dup own}','base-case.vmt':'#base base.vmt S{SAME local}','multiple-base.vmt':'#base base.vmt #base base2.vmt S{}','base-after.vmt':'S{} #base base.vmt','relative/start.vmt':'#include ../inc.vmt S{}','include-nested.vmt':'#include include-before.vmt S{top 1}','quoted-directive.vmt':'"#include" "inc.vmt" S{}','numeric-before.vmt':'A{v -1e-999} #include numeric-child.vmt B{v 2147483647}','numeric-child.vmt':'Child{v 2147483647}','numeric-included.vmt':'#include numeric-before.vmt A{v 2147483647}','case-path.vmt':'#INCLUDE INC.VMT S{}','relative/backslash.vmt':'#include "..\\INC.VMT" S{}','priority.vmt':'S{from high}'}
 for name,source in fixed.items():write(name,source)
 rng=random.Random(2026101701);keys=['same','SAME','group/item','nested','scalar','duplicate','$surfaceprop'];values=['metal','wood','0','2147483647','-0.0','0x0000000000000001','[1 2 3]','a\\b','']
 def body(depth=0):
  parts=[]
  for i in range(rng.randrange(1,7)):
   key=rng.choice(keys)
   if depth<2 and rng.randrange(4)==0:parts.append('"'+key+'"{'+body(depth+1)+'}')
   else:parts.append('"'+key+'" "'+rng.choice(values)+'"')
  return '\n'.join(parts)
 for i in range(160):
  folder='generated/'+str(i)+'/'
  write(folder+'leaf.vmt','Leaf{'+body()+'} ExtraLeaf{ignored 7}')
  write(folder+'base.vmt','#base leaf.vmt Base{'+body()+'}')
  write(folder+'inc.vmt','#base leaf.vmt Inc{'+body()+'} Other{'+body()+'}')
  write(folder+'root.vmt','#base base.vmt #include inc.vmt Root{'+body()+'} #base leaf.vmt Second{'+body()+'}')
 (low/'priority.vmt').write_text('S{from low}');(low/'inc.vmt').write_text('Low{from low}');cases=list(catalog)
 compare('controlled-graphs',[(high,'GAME',1),(low,'GAME',1)],catalog,cases)
 # Explicit real original filesystem head/tail/path-ID observations. Source
 # catalogs are selected independently by the test, not a runtime provider.
 for label,mounts,expected in [('tail-priority',[(low,'GAME',1),(high,'GAME',1)],'low'),('head-priority',[(high,'GAME',1),(low,'GAME',0)],'low'),('path-id',[(low,'OTHER',0),(high,'GAME',1)],'high')]:
  chosen={'priority.vmt':'S{from '+expected+'}'};observed=compare(label,mounts,chosen,['priority.vmt']);require(observed[0]['roots'][0]['children'][0]['value']==expected,'Priority witness')
 bsp=GAME/'tf_movies/maps/mtt_soldier.bsp';raw=bsp.read_bytes();o,size=struct.unpack_from('<II',raw,8+40*16);archive=zipfile.ZipFile(io.BytesIO(raw[o:o+size]));packed={e.filename.lower():archive.read(e).decode('ascii') for e in archive.infolist() if e.filename.lower().endswith('.vmt')};require(len(packed)==3,'Packed material count');packedName='materials/maps/mtt_soldier/glass/glasswindow002a_559_-1543_-340.vmt';override=high/packedName;override.parent.mkdir(parents=True,exist_ok=True);override.write_text('Override{from loose}')
 for label,mounts,source in [('pack-first',[(bsp,'GAME',1),(high,'GAME',1)],packed[packedName]),('loose-first',[(high,'GAME',1),(bsp,'GAME',1)],'Override{from loose}'),('pack-head',[(high,'GAME',1),(bsp,'GAME',0)],packed[packedName])]:compare(label,mounts,{packedName:source},[packedName])
 mounts=[Path(x) for x in (ROOT/'outputs/loose-collision-scene-20261016/verification-ub8mvw0l/mounts.txt').read_text().splitlines() if x];benchmark={};td,n=struct.unpack_from('<II',raw,8+2*16);st,_=struct.unpack_from('<II',raw,8+44*16);sd,_=struct.unpack_from('<II',raw,8+43*16)
 for i in range(n//32):
  index=struct.unpack_from('<I',raw,td+32*i+12)[0];off=struct.unpack_from('<I',raw,st+4*index)[0];a=sd+off;name='materials/'+raw[a:raw.index(0,a)].decode('ascii')+'.vmt';path=next((m/name for m in mounts if (m/name).is_file()),None)
  if name.lower() in packed:source=packed[name.lower()]
  else:require(path is not None,'Benchmark material missing '+name);source=path.read_bytes().decode('ascii');originals[str(path)]=sha(path)
  benchmark[name.lower()]=source
 require(len(benchmark)==87,'Benchmark referenced materials');benchmark.update(packed);compare('actual-benchmark-files',[(bsp,'GAME',0)]+[(m,'GAME',1) for m in mounts],benchmark,list(benchmark))
 write('missing-include.vmt','S{local retained} #include absent.vmt')
 negativeInput=run/'missing-native-input.bin';negativeOut=run/'missing-native.bin';negativeInput.write_bytes(struct.pack('<I',1)+text(str(high))+text('GAME')+struct.pack('<I',1)+struct.pack('<I',2)+text('absent.vmt')+text('GAME')+text('missing-include.vmt')+text('GAME'));negative=subprocess.run([str(OUT/'files_native_x86.exe'),str(libs),str(negativeInput),str(negativeOut)],cwd=run,capture_output=True,text=True,timeout=60);(run/'missing-native.log').write_text(negative.stdout+negative.stderr);require(negative.returncode==0,'Missing-file characterization exit');missing=decode(negativeOut.read_bytes());require(missing[0]['ok']==0 and missing[1]['ok']==1 and missing[1]['roots'][0]['children'][0]['value']=='retained','Original missing-file behavior');(run/'missing-behavior.json').write_text(json.dumps(missing,indent=2))
 for path,digest in originals.items():require(sha(Path(path))==digest,'Original changed '+path)
 result=dict(status='PASS_COMPARE',path=str(run),cases=sum(x['cases'] for x in scenarios),scenarios=scenarios,differences=0,executables=executables,original_hashes=originals,reference_receipts={str(PRIOR):sha(PRIOR)})
 (run/'comparison.json').write_text(json.dumps(result,indent=2));print('PASS_COMPARE '+str(run/'comparison.json'),flush=True)
 safety=OUT/'files_safety_x64.exe';binary=safety.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==0x8664 and b'asInvoker' in binary,'Safety executable architecture/manifest');imports=subprocess.run([str(DUMP),'/imports',str(safety)],capture_output=True,text=True,check=True).stdout;(run/'files_safety_x64-imports.txt').write_text(imports);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require(modules and set(x.lower() for x in modules)=={'kernel32.dll'},'Safety unexpected dependency');executables.append(dict(path=str(safety),sha256=sha(safety),machine='0x8664',direct_imports=modules));p=subprocess.run([str(safety)],cwd=run,capture_output=True,text=True,timeout=120);(run/'safety.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Safety failed '+p.stdout+p.stderr);print(p.stdout.strip(),flush=True)
 for path,digest in prior['sources'].items():require(sha(Path(path))==digest,'Existing project source changed '+path)
 for path,digest in originals.items():require(sha(Path(path))==digest,'Original changed '+path)
 builds=[]
 for name in ['native','owned','safety']:
  log=OUT/(name+'-build.log');require(not re.search(r'\b(?:warning|error) [A-Z]+\d+',log.read_text()),'Compiler diagnostic '+str(log));builds.append(dict(path=str(log),sha256=sha(log),warnings=0,errors=0))
 names=['source_material_keyvalues_files_candidate.h','probe_material_keyvalues_files_native.cpp','probe_material_keyvalues_files_owned.cpp','test_material_keyvalues_files.cpp','build_material_keyvalues_files.cmd','verify_material_keyvalues_files.py'];changed={str(ROOT/'work'/n):sha(ROOT/'work'/n) for n in names};sources=dict(changed);pending=[ROOT/'work'/n for n in names];visited=set()
 while pending:
  path=pending.pop()
  if path in visited:continue
  visited.add(path);sources[str(path)]=sha(path)
  if path.suffix in ['.cpp','.h']:
   for local in re.findall(r'^\s*#include\s*"([^"]+)"',path.read_text(),re.M):
    child=path.parent/local
    if child.is_file():pending.append(child)
 limits=[
  'Original oracle normally loads workspace copies of tier0/vstdlib/materialsystem/filesystem_stdio. Complete original LoadFromFile/LoadFromBuffer, actual VFileSystem017/IBaseFileSystem subobject, real files/mounts/BSP pak entries/KeyValues allocation/destruction. Original executable sections unchanged; no code patches/fake filesystem/private PE/GPU/editor startup. Explicit CRT errno reset before each requested file; uncontrolled live-process historical state not covered.',
  'Owned code resolves logical paths and ordered #include/#base graphs using an explicit caller-supplied read-only source provider. Its verifier builds independent immutable source catalogs from fixture/asset bytes. Catalogs contain no native outputs. Actual disk/mount/BSP ZIP source selection is NOT implemented by this new resolver; observed original mount priority is evidence for the next provider task.',
  '#include appends all included roots after local roots; #base merges first base-root children into first target root, compares keys case-sensitively, retains local values/types/duplicates and can add children to scalar nodes. Base files and directives execute in input order, with parse numeric range state across nested loads. Plain material patch include keys are not #include directives and remain unevaluated here.',
  'ASCII/raw checked parser constraints remain. Missing sources, include cycles, paths escaping logical root/absolute paths, files/depth/source-byte/node extents, unsupported conditions/assignments and files without a local root explicitly refuse without replacing prior published output. Original missing top-level files return false; original missing included files retain partial output (static warning call pinned; no console text captured), whereas owned code refuses that incomplete result.',
  'Multiple #include directives in one file intentionally refuse pending original ownership clarification. Exploratory original-only characterization returned damaged key names for a second appended include; actual append/cleanup instructions are pinned. That exploratory behavior is not treated as a deterministic success test or copied into owned code. Nested files with one include each are verified.',
  'All87 referenced benchmark material files plus2 other packed VMTs match raw resolved document output. Their plain patch include chains, effective shader parameters/defaults/fallbacks, conditions, filesystem archive decompression/provider and final IMaterial production remain incomplete. Not renderer/scene/SFM workflow parity.',
  'All immediate prior checkpoint sources and original hashes unchanged. No existing editor/document/save/render/animation modifications, lighting-guard removal, complete editor build/integration, dependencies installed, original/session writes or UI/SFM/Blender processes. Whole application remains PARTIALLY VERIFIED.'
 ]
 observation=OUT/'characterization';exploration={str(observation/name):sha(observation/name) for name in ['trees.json','native.log','input.bin','native.bin']}
 result.update(status='PASS',safety=p.stdout.strip(),executables=executables,build_logs=builds,changed_sources=changed,sources=sources,unchanged_prior_sources=len(prior['sources']),limitations=limits,original_missing_behavior=missing,original_missing_console_warning_captured="Couldn't load included" in negative.stdout+negative.stderr,exploratory_multiple_include_observation=exploration)
 receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2));(OUT/'source-integrity.json').write_text(json.dumps(dict(status='PASS',authoritative_receipt=str(receipt),receipt_sha256=sha(receipt),changed_sources=changed,sources=sources,executables=executables,build_logs=builds,original_hashes=originals),indent=2))
 lines=['VERIFIED KeyValues include/base resolver for explicit source provider.','Whole application PARTIALLY VERIFIED.',str(receipt),'Original/owned complete document comparisons='+str(result['cases'])+' differences=0',p.stdout.strip(),'Unchanged original hashes='+str(len(originals)),'Unchanged immediate prior source dependencies='+str(len(prior['sources'])),'','Exact freshly built/tested executables:']+[x['path']+' SHA256='+x['sha256'] for x in executables]+['','New sources:']+list(changed)+['','Limits:']+limits+['','Reproduce:','cmd /c work\\build_material_keyvalues_files.cmd','C:\\Users\\wigwo\\AppData\\Local\\Python\\bin\\python3.14.exe work\\verify_material_keyvalues_files.py']
 (OUT/'verification.txt').write_text('\n'.join(lines)+'\n');print('PASS '+str(receipt),flush=True)

if __name__=='__main__':main()
