"""Complete original material file loader and owned patch-chain comparison."""
import hashlib,json,struct,subprocess,tempfile,shutil,io,zipfile,random,re
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/material-patch-files-20261017'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
def require(ok,message):
 if not ok:raise RuntimeError(message)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def text(value):
 data=value.encode('ascii');return struct.pack('<I',len(data))+data
def request(mounts,cases):return struct.pack('<I',len(mounts))+b''.join(text(str(path))+text(id)+struct.pack('<I',position) for path,id,position in mounts)+struct.pack('<I',len(cases))+b''.join(text(name)+struct.pack('<I',mode) for name,mode in cases)
def decode(raw):
 at=0
 def word(fmt='<I'):
  nonlocal at
  v=struct.unpack_from(fmt,raw,at)[0];at+=struct.calcsize(fmt);return v
 def string():
  nonlocal at
  n=word();v=raw[at:at+n].decode('ascii');at+=n;return v
 def node():
  key=string();kind=word();value=string() if kind==1 else word() if kind in [2,3] else word('<Q') if kind in [4,7] else None;count=word();return dict(key=key,kind=kind,value=value,children=[node() for _ in range(count)])
 values=[]
 for _ in range(word()):
  ok=word();roots=[node() for _ in range(word())];aggregate=node();values.append(dict(ok=ok,roots=roots,aggregate=aggregate))
 require(at==len(raw),'Material patch wire span');return values

PRIOR=ROOT/'outputs/material-source-provider-20261017/verification-y14p2a_g/results.json'
DUMP=Path('C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/dumpbin.exe')
from verify_ifm_draw_packet import check_ranges
from verify_material_source_provider import bsp_bytes,zip_bytes
import decimal,math

def main():
 prior=json.loads(PRIOR.read_text());require(prior['status']=='PASS','Prior checkpoint');originals=dict(prior['original_hashes'])
 for path,digest in originals.items():require(sha(Path(path))==digest,'Original identity '+path)
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True);libs=run/'copied-libraries';libs.mkdir()
 for name in ['tier0.dll','vstdlib.dll','materialsystem.dll','filesystem_stdio.dll']:shutil.copyfile(GAME/'bin'/name,libs/name);require(sha(libs/name)==originals[str(GAME/'bin'/name)],'Copied library identity')
 im,evidence=check_ranges(GAME/'bin/materialsystem.dll','materialsystem-disassembly.txt',[('material_file_load',0x10006ba0,0x10006cb4),('patch_chain_load',0x10006970,0x10006b96),('patch_accumulate',0x10005100,0x100051c1),('aggregate_typed_copy',0x10004fe0,0x100050e4),('final_patch_apply',0x10004f80,0x10004fd4),('final_tree_assignment',0x10070e50,0x10071043),('connection_wrapper',0x10074fa0,0x10074fa9),('disconnect_wrapper',0x10074fb0,0x10074fb5),('normal_interface_connect',0x1006c200,0x1006c349),('normal_interface_disconnect',0x1006c350,0x1006c38d),('include_string_conversion',0x10071470,0x1007169e)])
 require(any(im.word(0x1010247c+8*i)==0x101255c4 and im.string(im.word(0x10102478+8*i))=='VFileSystem017' for i in range(56)),'Normal connector filesystem table');require(im.string(0x100ded58)=='%lld' and im.string(0x1009903c)=='%f' and im.string(0x10099040)=='%d','Include conversion formats');(run/'original-code-evidence.json').write_text(json.dumps(evidence,indent=2));executables=[]
 for name,machine in [('patch_files_native_x86',0x14c),('include_format_native_x86',0x14c),('patch_files_owned_x64',0x8664),('source_resolved_regression_x64',0x8664),('patch_files_safety_x64',0x8664)]:
  exe=OUT/(name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine and b'asInvoker' in binary,'Executable architecture/manifest');imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(name+'-imports.txt')).write_text(imports);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require(modules and set(x.lower() for x in modules)=={'kernel32.dll'},'Unexpected direct dependency');executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules))
 def execute(name,args,label):
  p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],cwd=run,capture_output=True,text=True,errors='replace',timeout=120);log=run/(label+'.log');log.write_text(p.stdout+p.stderr);require(p.returncode==0,'Program '+name+' exit '+str(p.returncode)+'; see '+str(log));return p.stdout+p.stderr
 scenarios=[]
 def compare(label,mounts,cases):
  inp=run/(label+'-input.bin');inp.write_bytes(request(mounts,cases));a=run/(label+'-native.bin');b=run/(label+'-owned.bin');log=execute('patch_files_native_x86',[libs,inp,a],label+'-native');require('KeyValues Error:' not in log and 'Failed to load' not in log,'Native material load diagnostics '+label);execute('patch_files_owned_x64',[inp,b],label+'-owned');native,owned=decode(a.read_bytes()),decode(b.read_bytes());require(len(native)==len(owned)==len(cases),'Output cases')
  if a.read_bytes()!=b.read_bytes():
   diff=[i for i,(x,y) in enumerate(zip(native,owned)) if x!=y];i=diff[0];p=run/(label+'-difference.json');p.write_text(json.dumps(dict(case=cases[i],native=native[i],owned=owned[i],indexes=diff),indent=2));raise RuntimeError('Patch file mismatch '+str(p))
  require(all(v['ok']==1 and v['roots'] and v['roots'][0]['key'].lower()!='patch' for v in native),'Unresolved positive material');scenarios.append(dict(name=label,cases=len(cases),input_sha256=sha(inp),output_sha256=sha(b),native_limit_warning='Infinite recursion in patch file?' in log));print('PASS '+label+' cases='+str(len(cases)),flush=True);return owned
 root=run/'loose';shutil.copytree(OUT/'characterization/mount',root);(root/'extra.vmt').write_bytes(b'Extra{from hash}');cases=[('base',0),('inner',0),('outer',0),('order',0),('game-relative',0),('duplicate',0),('empty',0),('multi-root',0),('multi-base',0),('include-int',0),('include-float',0),('include-uint64',0),('numeric-state',0),('alternate/base',1),('tie-positive',0),('tie-negative',0),('outer-multi',0),('outer-hash',0)]
 def write(path,source):
  p=root/path;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(source.encode('ascii'))
 write('materials/typed-skip.vmt','patch{include materials/base.vmt insert{wide 0x0000000000000001 newwide 0x1234567890ABCDEF overflow 1e99} replace{wide 0x0000000000000002}}');cases.append(('typed-skip',0))
 for count in range(1,11):
  for i in range(count):write('materials/chain'+str(count)+'-'+str(i)+'.vmt','patch{include materials/'+('base.vmt' if i+1==count else 'chain'+str(count)+'-'+str(i+1)+'.vmt')+' insert{chain v'+str(i)+'}}')
  cases.append(('chain'+str(count)+'-0',0))
 rng=random.Random(2026101702);values=['metal','wood','0','-1','123.5','-0.0','1e99','0x1234567890ABCDEF','[1 2 3]'];keys=['same','SAME','group/item','nested','scalar','dup','$surfaceprop','trailing/','double//leaf']
 def body(depth=0):
  parts=[]
  for i in range(rng.randrange(1,7)):
   key=rng.choice(keys)
   if depth<2 and rng.randrange(4)==0:parts.append('"'+key+'"{'+body(depth+1)+'}')
   else:parts.append('"'+key+'" "'+rng.choice(values)+'"')
  return '\n'.join(parts)
 for n in range(220):
  base='generated/'+str(n)+'/';write('materials/'+base+'base.vmt','Shader{'+body()+'} Extra{untouched yes}')
  levels=rng.randrange(1,5)
  for i in range(levels):write('materials/'+base+'patch'+str(i)+'.vmt','patch{include materials/'+base+('base.vmt' if i+1==levels else 'patch'+str(i+1)+'.vmt')+' insert{'+body()+'} replace{'+body()+'}}'+(' Peer{from patch}' if rng.randrange(4)==0 else ''))
  cases.append((base+'patch0',0))
 # Exact float32-to-fixed6 filenames independently prepared with Decimal.
 decimal.getcontext().prec=90;floats=[-0.,0.,.0078125,-.0078125,1.25,-1.25,struct.unpack('<f',bytes.fromhex('ffff7f7f'))[0],struct.unpack('<f',bytes.fromhex('01000000'))[0]]
 floats += [struct.unpack('<f',struct.pack('<f',k/128))[0] for k in range(-63,64)]
 while len(floats)<320:
  v=struct.unpack('<f',struct.pack('<I',rng.getrandbits(32)))[0]
  if math.isfinite(v):floats.append(v)
 context=decimal.Context(prec=17,rounding=decimal.ROUND_HALF_UP)
 def filename(value):return format(context.create_decimal_from_float(value).quantize(decimal.Decimal('0.000001'),rounding=decimal.ROUND_HALF_UP),'f')
 for i,value in enumerate(floats):
  name=filename(value);write(name,'Numeric{selected "chosen='+name+'"}');literal=format(value,'.9g');literal=literal if any(c in literal for c in '.eE') else literal+'.0';write('materials/float-'+str(i)+'.vmt','patch{include '+literal+'}');cases.append(('float-'+str(i),0))
 # Validate the independent Decimal17 fixture rule against whole original GetString.
 formatInput=run/'format-input.bin';formatOutput=run/'format-native.bin';formatInput.write_bytes(struct.pack('<I',len(floats))+b''.join(struct.pack('<f',v) for v in floats));execute('include_format_native_x86',[libs,formatInput,formatOutput],'format-native');data=formatOutput.read_bytes();at=4;nativeFormats=[]
 for _ in floats:n=struct.unpack_from('<I',data,at)[0];at+=4;nativeFormats.append(data[at:at+n].decode('ascii'));at+=n
 require(at==len(data) and nativeFormats==[filename(v) for v in floats],'Independent legacy17 formatting fixture')
 controlled=compare('controlled-patch-chains',[(root,'GAME',1)],cases)
 for i,value in enumerate(floats):
  name=filename(value);actual=controlled[len(cases)-len(floats)+i]['roots'][0]['children'][0]['value'];require(actual=='chosen='+name,'Independent include filename '+str(i))
 # Pak patch -> loose base and loose patch -> pak base, through real provider.
 packEntries=[('materials/packed-patch.vmt',b'patch{include materials/base.vmt replace{$surfaceprop metal}}'),('materials/packed-base.vmt',b'PackedShader{v packed $surfaceprop glass}')];pak=run/'controlled.bsp';pak.write_bytes(bsp_bytes(zip_bytes(packEntries)));write('materials/loose-patch.vmt','patch{include materials/packed-base.vmt replace{v loose}}');compare('cross-bsp-patch-chains',[(pak,'GAME',0),(root,'GAME',1)],[('packed-patch',0),('loose-patch',0)])
 # Actual benchmark raw materials now include real material-patch resolution.
 bsp=GAME/'tf_movies/maps/mtt_soldier.bsp';raw=bsp.read_bytes();td,n=struct.unpack_from('<II',raw,8+2*16);st,_=struct.unpack_from('<II',raw,8+44*16);sd,_=struct.unpack_from('<II',raw,8+43*16);names=[]
 for i in range(n//32):index=struct.unpack_from('<I',raw,td+32*i+12)[0];off=struct.unpack_from('<I',raw,st+4*index)[0];a=sd+off;names.append(raw[a:raw.index(0,a)].decode('ascii'))
 o,size=struct.unpack_from('<II',raw,8+40*16);archive=zipfile.ZipFile(io.BytesIO(raw[o:o+size]));names.extend(e.filename[len('materials/'):-4] for e in archive.infolist() if e.filename.lower().endswith('.vmt') and e.filename[len('materials/'):-4] not in names);mounts=[Path(x) for x in (ROOT/'outputs/loose-collision-scene-20261016/verification-ub8mvw0l/mounts.txt').read_text().splitlines() if x];actual=compare('actual-benchmark-materials',[(bsp,'GAME',0)]+[(m,'GAME',1) for m in mounts],[(n,0) for n in names]);require(len(names)==89,'Benchmark coverage')
 # Include source identities outside prior receipt are checked, not written.
 base=GAME/'hl2/materials/glass/glasswindow002a.vmt';originals[str(base)]=sha(base)
 # Recompile the old actual-source graph test against the scoped context API
 # extension, preserving all758 previously native-verified result bytes.
 regression=[];oldRun=Path(prior['path'])
 for scenario in prior['scenarios']:
  if scenario['kind']!='resolved':continue
  name=scenario['name'];inp=oldRun/(name+'-input.bin');require(sha(inp)==scenario['input_sha256'],'Prior graph request identity');dest=run/(name+'-regression.bin');execute('source_resolved_regression_x64',[inp,dest],name+'-regression');require(dest.read_bytes()==(oldRun/(name+'-native.bin')).read_bytes(),'Default-context graph regression '+name);regression.append(dict(name=name,cases=scenario['cases'],output_sha256=sha(dest)))
 for path,digest in originals.items():require(sha(Path(path))==digest,'Original changed '+path)
 result=dict(status='PASS_COMPARE',path=str(run),cases=sum(x['cases'] for x in scenarios),scenarios=scenarios,regressions=regression,regression_cases=sum(x['cases'] for x in regression),float_include_cases=len(floats),differences=0,executables=executables,original_hashes=originals,reference_receipts={str(PRIOR):sha(PRIOR)})
 (run/'comparison.json').write_text(json.dumps(result,indent=2));print('PASS_COMPARE '+str(run/'comparison.json'),flush=True)
 safety=execute('patch_files_safety_x64',[],'safety');print(safety.strip(),flush=True)
 modified={'source_material_keyvalues_files_candidate.h'}
 for path,digest in prior['sources'].items():
  if Path(path).name not in modified:require(sha(Path(path))==digest,'Unrelated source changed '+path)
 builds=[]
 for name in ['native','format','owned','regression','safety']:
  log=OUT/(name+'-build.log');require(not re.search(r'\b(?:warning|error) [A-Z]+\d+',log.read_text()),'Compiler diagnostic '+str(log));builds.append(dict(path=str(log),sha256=sha(log),warnings=0,errors=0))
 names=['source_material_patch_files_candidate.h','probe_material_patch_files_native.cpp','probe_material_patch_files_owned.cpp','probe_material_include_format_native.cpp','test_material_patch_files.cpp','build_material_patch_files.cmd','verify_material_patch_files.py','source_material_keyvalues_files_candidate.h'];changed={str(ROOT/'work'/n):sha(ROOT/'work'/n) for n in names};sources=dict(changed);pending=[ROOT/'work'/n for n in names]+[ROOT/'work/probe_material_source_owned.cpp'];visited=set()
 while pending:
  path=pending.pop()
  if path in visited:continue
  visited.add(path);sources[str(path)]=sha(path)
  if path.suffix in ['.cpp','.h']:
   for local in re.findall(r'^\s*#include\s*"([^"]+)"',path.read_text(),re.M):
    child=path.parent/local
    if child.is_file():pending.append(child)
 limits=[
  'Native original whole material loader6BA0,chain6970,aggregate5100/value-copy4FE0,final apply4F80 andassignment70E50 run with normal workspace library initialization and actual filesystem. Original normal interface connector74FA0->6C200 binds VFileSystem017 from real filesystem factory; normal disconnect74FB0->6C350 restores original filesystem binding. No fixture data-global assignment,fake services,code patches/private PE/GPU/editor startup. Original material/filesystem executable sections unchanged. Optional original dependency-index vector passed null; cache/index registration not tested.',
  'Owned native64 loads actual sources via prior loose/stored-BSP provider and KeyValues graph resolver. Plain patch includes are GAME-root logical paths, not relative to patch directory and not auto-appended .vmt. Root name formatting appends .vmt and optionally materials/. Patch sections combine outer-to-inner, inner overrides earlier accumulated values, separate scalar/block passes preserve native ordering, duplicate/path/case rules. Original final copy rebuilds child-bearing hybrid nodes as blocks; includes retained peers on single-local-root reloads. Shader defaults/fallbacks/condition interpretation/effective IMaterial variables not covered.',
  'Raw base uint64/infinity retained; aggregate helper skips uint64 writes, accepts native float infinity writes. Existing older patch-only public validator remains unchanged/restricted. Include names coerce int32/signed64/finitefloat32 using observed legacy formats;320 original GetString cases independently match Decimal17 significant digits plus fixed6 half-away rounding. Nonfinite include-name conversion explicitly refused. No exhaustive claim over all original CRT locale/historic process state.',
  'Missing/invalid sources,cycles,unresolved chains after10 includes,unsafe/extreme paths,unsupported trees,source-byte/read/node extents refuse and preserve prior published result. Original whole loader may return true with unresolved original patch on included-file failures; owned code intentionally refuses incomplete material. Exactly10 resolved layers accepted (original also emits its limit warning); original partial recovery or unsafe diagnostic filename lifetime is not emulated.',
  'Scoped existing graph header extension carries initial numeric-range context and reports single-local-root peer-retention metadata. Default behavior verified by freshly recompiled actual-source graph probe against all758 prior frozen native results. No existing document/history/render/animation/save ownership changes. All other immediate prior source dependencies unchanged.',
  'All89 benchmark material files,including2 packed glass patches resolving actual installed base material,match original complete raw patched documents and patch accumulators. This is not shader/material rendering equivalence. Source provider compressed/VPK/implicit archive/other checked refusals remain; prior child-reparse runtime check remains UNVERIFIED because Windows denied fixture creation.',
  'No original/session writes,dependencies installed,SFM/Blender/UI sessions,lighting guard removal,editor integration/full editor build,renderer/animation/document/save/serialization changes or whole-project completion. Whole application PARTIALLY VERIFIED; full open/evaluate/display/edit/save-copy/reopen fidelity incomplete.'
 ]
 result.update(status='PASS',safety=safety.strip(),executables=executables,build_logs=builds,changed_sources=changed,sources=sources,unchanged_prior_sources=sum(Path(path).name not in modified for path in prior['sources']),limitations=limits)
 receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2));(OUT/'source-integrity.json').write_text(json.dumps(dict(status='PASS',authoritative_receipt=str(receipt),receipt_sha256=sha(receipt),changed_sources=changed,sources=sources,executables=executables,build_logs=builds,original_hashes=originals),indent=2))
 lines=['VERIFIED owned native64 material file/patch-chain resolution in checked scope.','Whole application PARTIALLY VERIFIED.',str(receipt),'Original/owned complete material document+accumulator cases='+str(result['cases'])+' differences=0','Original GetString vs independent legacy numeric formatting='+str(result['float_include_cases']),'Unchanged default-context frozen graph cases='+str(result['regression_cases']),safety.strip(),'Unchanged original hashes='+str(len(originals)),'Unchanged other immediate prior sources='+str(result['unchanged_prior_sources']),'','Exact freshly built/tested executables:']+[x['path']+' SHA256='+x['sha256'] for x in executables]+['','New/modified sources:']+list(changed)+['','Limits:']+limits+['','Reproduce:','cmd /c work\\build_material_patch_files.cmd','C:\\Users\\wigwo\\AppData\\Local\\Python\\bin\\python3.14.exe work\\verify_material_patch_files.py'];(OUT/'verification.txt').write_text('\n'.join(lines)+'\n');print('PASS '+str(receipt),flush=True)

if __name__=='__main__':main()
