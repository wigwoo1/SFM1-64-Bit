"""Original material patch policy on typed KeyValues, no effective VMT claim."""
import collections,hashlib,io,json,math,random,re,shutil,struct,subprocess,tempfile,zipfile
from pathlib import Path
from verify_ifm_draw_packet import check_ranges
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/material-patch-policy-20261017'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
PRIOR=ROOT/'outputs/world-surface-table-20261017/verification-pa0f0say/results.json'
DUMP=Path('C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/dumpbin.exe')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
 if not ok:raise RuntimeError(message)
def text(s):
 b=s.encode('ascii');return struct.pack('<I',len(b))+b
def node(key,kind=0,value=None,children=None):return dict(key=key,kind=kind,value=value,children=children or [])
def encode(n):
 raw=text(n['key'])+struct.pack('<I',n['kind']);kind=n['kind']
 if kind==1:raw+=text(n['value'])
 elif kind in [2,3]:raw+=struct.pack('<I',n['value'])
 elif kind==4:raw+=struct.pack('<Q',n['value'])
 return raw+struct.pack('<I',len(n['children']))+b''.join(encode(c) for c in n['children'])
def decode_nodes(raw):
 at=0
 def word(fmt='<I'):
  nonlocal at
  v=struct.unpack_from(fmt,raw,at)[0];at+=struct.calcsize(fmt);return v
 def string():
  nonlocal at
  n=word();v=raw[at:at+n].decode('ascii');at+=n;return v
 def tree():
  key=string();kind=word();value=string() if kind==1 else word() if kind in [2,3] else word('<Q') if kind==4 else None;count=word();return node(key,kind,value,[tree() for _ in range(count)])
 count=word();values=[tree() for _ in range(count)];require(at==len(raw),'Node wire span');return values

def main():
 prior=json.loads(PRIOR.read_text());require(prior['status']=='PASS','Prior verified checkpoint');originals=dict(prior['original_hashes'])
 for n in ['tier0.dll','vstdlib.dll','materialsystem.dll']:
  p=GAME/'bin'/n;originals[str(p)]=sha(p)
 for p,s in originals.items():require(sha(Path(p))==s,'Original identity '+p)
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True);libraries=run/'copied-libraries';libraries.mkdir()
 for n in ['tier0.dll','vstdlib.dll','materialsystem.dll']:shutil.copyfile(GAME/'bin'/n,libraries/n);require(sha(libraries/n)==originals[str(GAME/'bin'/n)],'Copied library identity')
 image,evidence=check_ranges(GAME/'bin/materialsystem.dll','materialsystem-disassembly.txt',[('patch_value_copy',0x10004de0,0x10004f63),('apply_insert_then_replace',0x10004f80,0x10004fd4),('KV_constructor',0x10070a10,0x10070a79),('KV_append',0x10070c00,0x10070c2d),('KV_delete_this',0x10071280,0x1007129e)])
 require(image.string(0x10095a90)=='insert' and image.string(0x10095a88)=='replace','Patch keyword identity');(run/'original-code-evidence.json').write_text(json.dumps(evidence,indent=2));cases=[];kinds=collections.Counter()
 # All active scalar types and retained children, with actual overwrite and
 # missing-key witnesses. Value bits carry exact finite float representations.
 vals={0:None,1:'wood',2:0xffffff83,3:struct.unpack('<I',struct.pack('<f',-.25))[0],4:0x12345678}
 for baseKind in range(5):
  for sourceKind in range(5):
   base=node('LightmappedGeneric',children=[node('value',baseKind,vals[baseKind],[node('old',1,'retained')])]);patch=node('patch',children=[node('insert',children=[node('value',sourceKind,vals[sourceKind],[node('new',1,'added')])])]);cases.append((base,patch));kinds['type_pair']+=1
 base=node('LightmappedGeneric',children=[node('$surfaceprop',1,'wood')]);patch=node('patch',children=[node('insert',children=[node('$surfaceprop',1,'metal'),node('$new',1,'added')]),node('replace',children=[node('$missing',1,'ignored')])]);cases.append((base,patch));overwrite=len(cases)-1
 for replace in [False,True]:
  for key in ['value','VALUE','group/item','/leading','trailing/','double//leaf','']:
   for nested in [False,True]:
    base=node('shader',children=[node('Value',1,'first'),node('value',1,'second'),node('group',children=[node('item',1,'old')])]);child=node(key,children=[node('sub',1,'nested')]) if nested else node(key,1,'written');patch=node('PATCH',children=[node('replace' if replace else 'insert',children=[child])]);cases.append((base,patch));kinds['path_case_duplicate']+=1
 rng=random.Random(20261017);keys=['$surfaceprop','$surfaceprop2','$envmap','value','VALUE','nested','leaf','group/item','','same','same']
 def random_node(depth=0,key=None):
  kind=rng.randrange(5);value=None if kind==0 else rng.choice(['','metal','Wood','missing','a/b','123']) if kind==1 else rng.getrandbits(32) if kind in [2,4] else struct.unpack('<I',struct.pack('<f',rng.choice([-0.,0.,.125,-1.,123.25])))[0];children=[random_node(depth+1) for _ in range(rng.randrange(4))] if depth<3 else [];return node(rng.choice(keys) if key is None else key,kind,value,children)
 for i in range(1500):
  base=node('base',children=[random_node() for _ in range(rng.randrange(1,5))]);patch=node('patch',children=[node(rng.choice(['insert','replace','INSERT','REPLACE','unused']),children=[random_node() for _ in range(rng.randrange(4))]) for _ in range(rng.randrange(1,4))]);cases.append((base,patch));kinds['random_typed_hybrids']+=1
 # Real packed glass patch is represented as typed input, with source text
 # retained independently. This tests patch semantics, not file parsing.
 raw=(GAME/'tf_movies/maps/mtt_soldier.bsp').read_bytes();o,n=struct.unpack_from('<II',raw,8+40*16);archive=zipfile.ZipFile(io.BytesIO(raw[o:o+n]));packed='materials/maps/mtt_soldier/glass/glasswindow002a_559_-1543_-340.vmt';patchSource=archive.read(packed).decode('ascii');basePath=GAME/'hl2/materials/glass/glasswindow002a.vmt';originals[str(basePath)]=sha(basePath);baseSource=basePath.read_text();require('"$surfaceprop" "glass"' in baseSource and '"$envmap"' in patchSource,'Actual glass fixture declarations');cases.append((node('LightmappedGeneric',children=[node('$basetexture',1,'Glass/glasswindow002a'),node('$surfaceprop',1,'glass')]),node('patch',children=[node('include',1,'materials/GLASS/GLASSWINDOW002A.vmt'),node('replace',children=[node('$envmap',1,'maps/mtt_soldier/c559_-1543_-340')])])));realGlass=len(cases)-1;kinds['real_packed_patch_typed_fixture']+=1
 inp=run/'input.bin';inp.write_bytes(struct.pack('<I',len(cases))+b''.join(encode(a)+encode(b) for a,b in cases));results=[];executables=[]
 for name,machine in [('patch_native_x86',0x14c),('patch_owned_x64',0x8664)]:
  exe=OUT/(name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine and b'asInvoker' in binary,'Executable architecture/manifest');imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(name+'-imports.txt')).write_text(imports);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require(modules and set(x.lower() for x in modules)=={'kernel32.dll'},'Unexpected direct dependency');out=run/(name+'.bin');p=subprocess.run([str(exe),str(libraries),str(inp),str(out)],cwd=run,capture_output=True,text=True,timeout=120);(run/(name+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,'Probe failed '+str(run)+' '+name+' exit='+str(p.returncode)+' '+p.stdout+p.stderr);print(p.stdout.strip(),flush=True);results.append(out.read_bytes());executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules))
 native,owned=map(decode_nodes,results);require(len(native)==len(owned)==len(cases),'Output cases')
 if results[0]!=results[1]:
  first=next(i for i,(a,b) in enumerate(zip(native,owned)) if a!=b);(run/'difference.json').write_text(json.dumps(dict(index=first,base=cases[first][0],patch=cases[first][1],native=native[first],owned=owned[first]),indent=2));raise RuntimeError('Patch mismatch '+str(run/'difference.json'))
 require(native[overwrite]['children'][0]['value']=='metal' and not any(c['key']=='$missing' for c in native[overwrite]['children']),'Overwrite/missing witness');require(native[realGlass]['children']==cases[realGlass][0]['children'],'Real glass replace missing envmap must preserve base surface data')
 for p,s in originals.items():require(sha(Path(p))==s,'Original changed after test '+p)
 # Successful compare receipt is saved now; final evidence/report is extended
 # with separate owned safety checks when the bounded task completes.
 result=dict(status='PASS_COMPARE',path=str(run),cases=len(cases),coverage=dict(kinds),differences=0,fixture_sha256=sha(inp),output_sha256=hashlib.sha256(results[1]).hexdigest(),executables=executables,original_hashes=originals,actual_glass_patch=dict(path=packed,source=patchSource,base_path=str(basePath),base_source=baseSource,scope='typed fixture; not a VMT parser or full include/shader resolver'),reference_receipts={str(PRIOR):sha(PRIOR)})
 (run/'comparison.json').write_text(json.dumps(result,indent=2));print('PASS_COMPARE '+str(run/'comparison.json'),flush=True)
 safety=OUT/'patch_safety_x64.exe';binary=safety.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==0x8664 and b'asInvoker' in binary,'Safety executable architecture/manifest');imports=subprocess.run([str(DUMP),'/imports',str(safety)],capture_output=True,text=True,check=True).stdout;(run/'patch_safety_x64-imports.txt').write_text(imports);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require(modules and set(x.lower() for x in modules)=={'kernel32.dll'},'Safety unexpected dependency');executables.append(dict(path=str(safety),sha256=sha(safety),machine='0x8664',direct_imports=modules));p=subprocess.run([str(safety)],cwd=run,capture_output=True,text=True,timeout=120);(run/'safety.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Safety failed '+p.stdout+p.stderr);print(p.stdout.strip(),flush=True)
 for path,digest in prior['sources'].items():require(sha(Path(path))==digest,'Existing project source changed '+path)
 for path,digest in originals.items():require(sha(Path(path))==digest,'Original changed '+path)
 builds=[]
 for name in ['native','owned','safety']:
  log=OUT/(name+'-build.log');require(not re.search(r'\b(?:warning|error) [A-Z]+\d+',log.read_text()),'Compiler diagnostic '+str(log));builds.append(dict(path=str(log),sha256=sha(log),warnings=0,errors=0))
 names=['source_material_patch_policy_candidate.h','probe_material_patch_policy.cpp','test_material_patch_policy.cpp','build_material_patch_policy.cmd','verify_material_patch_policy.py'];changed={str(ROOT/'work'/n):sha(ROOT/'work'/n) for n in names};sources=dict(changed);pending=[ROOT/'work'/n for n in names];visited=set()
 while pending:
  path=pending.pop()
  if path in visited:continue
  visited.add(path);sources[str(path)]=sha(path)
  if path.suffix in ['.cpp','.h']:
   for local in re.findall(r'^\s*#include\s*"([^"]+)"',path.read_text(),re.M):
    child=path.parent/local
    if child.is_file():pending.append(child)
 limits=['Original oracle normally loads workspace copies of tier0/vstdlib/materialsystem, constructs real original KeyValues with original allocator/typed setters/append, runs whole original patch functions and original destruction. Original material code sections unchanged; no fake KeyValues/material service or engine code hooks.', 'Typed KeyValues inputs supplied explicitly. Raw VMT parsing, conditions, include/patch chains, BSP archive/mount precedence, material shader defaults/fallbacks and final effective IMaterial parameter production remain unverified.', 'Owned checked subset covers block/string/int32/finite float32/opaque pointer identity, duplicate/case/path keys, retained child state and original empty-target marker; other KeyValues types/non-ASCII/unbounded input explicitly reject. Native pointer comparisons restricted32bits; separate owned safety proves64bit opaque identity preservation.', 'Packed glass patch and installed base VMT are represented as a typed fixture with exact source provenance; this is not a full benchmark material-load comparison.', 'All prior project source dependencies remain byte-identical. No renderer/animation/document/save/serialization changes, lighting guard removal, editor build/integration, original writes, dependencies installed, SFM/Blender/UI processes or whole-project completion claim.']
 result.update(status='PASS',safety=p.stdout.strip(),executables=executables,build_logs=builds,changed_sources=changed,sources=sources,unchanged_prior_sources=len(prior['sources']),limitations=limits)
 receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2));(OUT/'source-integrity.json').write_text(json.dumps(dict(status='PASS',authoritative_receipt=str(receipt),receipt_sha256=sha(receipt),changed_sources=changed,sources=sources,executables=executables,build_logs=builds,original_hashes=originals),indent=2));lines=['VERIFIED typed original material patch policy within checked subset.','Whole application PARTIALLY VERIFIED.',str(receipt),'Original/owned typed patch cases='+str(len(cases))+' differences=0',p.stdout.strip(),'Unchanged original hashes='+str(len(originals)),'Unchanged prior source dependencies='+str(len(prior['sources'])),'','Exact freshly built/tested executables:']+[x['path']+' SHA256='+x['sha256'] for x in executables]+['','New sources:']+list(changed)+['','Limits:']+limits+['','Reproduce:','cmd /c work\\build_material_patch_policy.cmd','C:\\Users\\wigwo\\AppData\\Local\\Python\\bin\\python3.14.exe work\\verify_material_patch_policy.py'];(OUT/'verification.txt').write_text('\n'.join(lines)+'\n');print('PASS '+str(receipt),flush=True)
if __name__=='__main__':main()
