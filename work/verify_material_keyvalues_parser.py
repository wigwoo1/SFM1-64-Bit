"""Raw material KeyValues reader vs actual original LoadFromBuffer.
File/source providers, directives, symbols and shader processing remain separate.
"""
import collections,hashlib,io,json,random,re,shutil,struct,subprocess,tempfile,zipfile
from pathlib import Path
from verify_ifm_draw_packet import check_ranges
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/material-keyvalues-parser-20261017'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
PRIOR=ROOT/'outputs/material-patch-policy-20261017/verification-tlefvd74/results.json'
DUMP=Path('C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/dumpbin.exe')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
 if not ok:raise RuntimeError(message)
def text(s):
 raw=s.encode('ascii');return struct.pack('<I',len(raw))+raw
def decode(raw):
 at=0
 def word(fmt='<I'):
  nonlocal at
  v=struct.unpack_from(fmt,raw,at)[0];at+=struct.calcsize(fmt);return v
 def string():
  nonlocal at
  n=word();v=raw[at:at+n].decode('ascii');at+=n;return v
 def node():
  key=string();kind=word();value=string() if kind==1 else word() if kind in [2,3] else word('<Q') if kind in [4,7] else None;n=word();return dict(key=key,kind=kind,value=value,children=[node() for _ in range(n)])
 out=[]
 for _ in range(word()):out.append(dict(ok=word(),roots=[node() for _ in range(word())]))
 require(at==len(raw),'Raw parser wire span');return out
def main():
 prior=json.loads(PRIOR.read_text());require(prior['status']=='PASS','Previous checkpoint');originals=dict(prior['original_hashes'])
 for p,s in originals.items():require(sha(Path(p))==s,'Original identity '+p)
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True);libraries=run/'copied-libraries';libraries.mkdir()
 for name in ['tier0.dll','vstdlib.dll','materialsystem.dll']:shutil.copyfile(GAME/'bin'/name,libraries/name);require(sha(libraries/name)==originals[str(GAME/'bin'/name)],'Copied library identity')
 image,evidence=check_ranges(GAME/'bin/materialsystem.dll','materialsystem-disassembly.txt',[('raw_LoadFromBuffer',0x100720a0,0x10072516),('body_tokenization_and_scalar_selection',0x100717c0,0x10071c9a),('token_reader',0x10070620,0x100707d4),('line_comment_reader',0x1006f880,0x1006f8f9),('borrowed_buffer_constructor',0x1006f6d0,0x1006f749)])
 (run/'original-code-evidence.json').write_text(json.dumps(evidence,indent=2));cases=[];sources=[]
 def add(name,source,kind):cases.append((name,source,kind))
 raw=(GAME/'tf_movies/maps/mtt_soldier.bsp').read_bytes();lump=lambda id:struct.unpack_from('<II',raw,8+16*id);td,n=lump(2);st,_=lump(44);sd,_=lump(43);o,size=lump(40);archive=zipfile.ZipFile(io.BytesIO(raw[o:o+size]));mounts=[Path(x) for x in (ROOT/'outputs/loose-collision-scene-20261016/verification-ub8mvw0l/mounts.txt').read_text().splitlines() if x];referenced=[]
 for i in range(n//32):
  index=struct.unpack_from('<I',raw,td+32*i+12)[0];off=struct.unpack_from('<I',raw,st+4*index)[0];a=sd+off;name=raw[a:raw.index(0,a)].decode('ascii');path=next((m/'materials'/(name+'.vmt') for m in mounts if (m/'materials'/(name+'.vmt')).is_file()),None)
  if path:data=path.read_bytes();originals[str(path)]=sha(path);provenance=str(path)
  else:entry=next(v for v in archive.infolist() if v.filename.lower()==('materials/'+name+'.vmt').lower());data=archive.read(entry);provenance='BSP-pak:'+entry.filename
  source=data.decode('ascii');add(name,source,'benchmark-vmt');sources.append(dict(name=name,source=provenance,sha256=hashlib.sha256(data).hexdigest()));referenced.append(provenance.lower())
 for entry in archive.infolist():
  if entry.filename.lower().endswith('.vmt') and ('bsp-pak:'+entry.filename).lower() not in referenced:add(entry.filename,archive.read(entry).decode('ascii'),'extra-packed-vmt')
 initial=(OUT/'capture-sources.txt').read_text().splitlines()
 # Keep captured whitespace-containing numeric source intact; each capture
 # source was explicitly written on one line in the initial ABI investigation.
 for i,source in enumerate(initial):add('capture-'+str(i),source,'ABI-characterization')
 values=['0','-0','+0','1','-1','00001','+12',' 12','12 ','2147483647','-2147483648','2147483648','-2147483649','4294967295','0.0','-0.0','.5','+.25','1.','1e4','-1e4','1e-50','-1e-50','1e99','-1e99','1e309','-1e309','1e-999','-1e-999','-1e-310','1e-310','-2.225073858507201e-308','2.225073858507201e-308','-2.2250738585072014e-308','2.2250738585072014e-308','16777217.0','0.100000001490116119384765625','0x0000000000000001','0xFFFFFFFFFFFFFFFF','0x1234567890ABCDEF','0X1234567890abcdef','0x10','0x1.0p1','nan','NAN','inf','INF','1.#INF','1.#IND','-1.#INF','12.0suffix','metal','[ 1 2 3 ]','{literal}','http://example/path','a\\b\\c','#value','']
 for i,value in enumerate(values):add('scalar-'+str(i),'S{value "'+value+'"}','scalar-boundary')
 for shift in range(12):
  sequence=values[shift:]+values[:shift];add('mixed-numeric-'+str(shift),'S{'+''.join('v'+str(i)+' "'+value+'"\n' for i,value in enumerate(sequence))+'}','stateful-numeric-sequence')
 rng=random.Random(20261017)
 def body(depth=0):
  out=[]
  for i in range(rng.randrange(1,9)):
   key=rng.choice(['a','A','duplicate','nested','$surfaceprop','360?$color2','path/key','slash\\key'])
   if depth<3 and rng.randrange(5)==0:out.append('"'+key+'"{'+body(depth+1)+'}')
   else:out.append('"'+key+'" "'+rng.choice(values)+'"')
  return '\n'.join(out)
 for i in range(256):add('generated-'+str(i),'// comment\r\n"Shader"{'+body()+'}\n// tail','generated-valid')
 add('empty-blocks','One{} Two{empty{} empty{} same first SAME second}','ordered-duplicate-multiple-root')
 add('vector-symbol-literals','S{ v "[1 2 3]" quoted "{}" equals "a=b" comment "// text" hash "#base" }','quoted-delimiters')
 add('max-token','S{v "'+'x'*32767+'"}','token-boundary')
 inp=run/'input.bin';inp.write_bytes(struct.pack('<I',len(cases))+b''.join(text(name)+text(source) for name,source,_ in cases));executables=[];results=[];logs=[]
 for name,machine in [('parser_native_x86',0x14c),('parser_owned_x64',0x8664)]:
  exe=OUT/(name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine and b'asInvoker' in binary,'Executable machine/manifest');imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(name+'-imports.txt')).write_text(imports);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require(modules and set(x.lower() for x in modules)=={'kernel32.dll'},'Unexpected direct import');executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules));out=run/(name+'.bin');p=subprocess.run([str(exe),str(libraries),str(inp),str(out)],cwd=run,capture_output=True,text=True,timeout=120);(run/(name+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,'Parser failed '+name+' exit='+str(p.returncode)+'; see '+str(run/(name+'.log')));require('KeyValues Error:' not in p.stdout+p.stderr,'Original parser diagnostics; see '+str(run/(name+'.log')));print(next((line for line in reversed(p.stdout.splitlines()) if line.startswith('PASS ')),name+' completed'),flush=True);results.append(out.read_bytes())
 native,owned=map(decode,results);require(len(native)==len(owned)==len(cases),'Document count')
 if results[0]!=results[1]:
  diff=[i for i,(a,b) in enumerate(zip(native,owned)) if a!=b];first=diff[0];(run/'difference.json').write_text(json.dumps(dict(index=first,name=cases[first][0],source=cases[first][1],native=native[first],owned=owned[first],different_cases=diff),indent=2));raise RuntimeError('Raw parser difference '+str(run/'difference.json'))
 def visit(node):
  yield node
  for c in node['children']:yield from visit(c)
 types=collections.Counter(n['kind'] for doc in native for root in doc['roots'] for n in visit(root));require(set(types)=={0,1,2,3,7},'Typed parser coverage');require(sum(k=='benchmark-vmt' for _,_,k in cases)==87,'Benchmark coverage')
 for path,digest in originals.items():require(sha(Path(path))==digest,'Original changed '+path)
 result=dict(status='PASS_COMPARE',path=str(run),cases=len(cases),categories=dict(collections.Counter(k for _,_,k in cases)),types=dict(types),differences=0,input_sha256=sha(inp),output_sha256=hashlib.sha256(results[1]).hexdigest(),benchmark_sources=sources,executables=executables,original_hashes=originals,reference_receipts={str(PRIOR):sha(PRIOR)})
 (run/'comparison.json').write_text(json.dumps(result,indent=2));print('PASS_COMPARE '+str(run/'comparison.json'),flush=True)
 safety=OUT/'parser_safety_x64.exe';binary=safety.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==0x8664 and b'asInvoker' in binary,'Safety executable architecture/manifest');imports=subprocess.run([str(DUMP),'/imports',str(safety)],capture_output=True,text=True,check=True).stdout;(run/'parser_safety_x64-imports.txt').write_text(imports);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require(modules and set(x.lower() for x in modules)=={'kernel32.dll'},'Safety unexpected dependency');executables.append(dict(path=str(safety),sha256=sha(safety),machine='0x8664',direct_imports=modules));p=subprocess.run([str(safety)],cwd=run,capture_output=True,text=True,timeout=120);(run/'safety.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Safety failed '+p.stdout+p.stderr);print(p.stdout.strip(),flush=True)
 for path,digest in prior['sources'].items():require(sha(Path(path))==digest,'Existing project source changed '+path)
 for path,digest in originals.items():require(sha(Path(path))==digest,'Original changed '+path)
 builds=[]
 for name in ['native','owned','safety']:
  log=OUT/(name+'-build.log');require(not re.search(r'\b(?:warning|error) [A-Z]+\d+',log.read_text()),'Compiler diagnostic '+str(log));builds.append(dict(path=str(log),sha256=sha(log),warnings=0,errors=0))
 names=['native_material_keyvalues_fixture.h','source_material_keyvalues_parser_candidate.h','probe_material_keyvalues_parser.cpp','test_material_keyvalues_parser.cpp','build_material_keyvalues_parser.cmd','verify_material_keyvalues_parser.py'];changed={str(ROOT/'work'/n):sha(ROOT/'work'/n) for n in names};sources=dict(changed);pending=[ROOT/'work'/n for n in names];visited=set()
 while pending:
  path=pending.pop()
  if path in visited:continue
  visited.add(path);sources[str(path)]=sha(path)
  if path.suffix in ['.cpp','.h']:
   for local in re.findall(r'^\s*#include\s*"([^"]+)"',path.read_text(),re.M):
    child=path.parent/local
    if child.is_file():pending.append(child)
 limits=[
  'Actual original raw LoadFromBuffer executes from normally initialized workspace copies of tier0/vstdlib/materialsystem; complete original parser, real KeyValues allocation/extraction/destruction, borrowed guarded CUtlBuffer. No code patches, fake KeyValues, private PE loader, GPU/editor/SFM startup or external filesystem service. Material-system executable sections and borrowed source bytes/guards checked unchanged.',
  'The fixture explicitly resets original CRT errno to zero before each document. Within-document numeric range state is retained and affects later int-boundary conversion. Owned caller may explicitly supply initial range state; uncontrolled original process-wide historical errno/locale state is not covered.',
  'Raw parse subset: ASCII, disabled quote escapes, double-slash comments, ordered duplicate/literal-path keys, multiple roots, block/string/int32/float32/uint64 values. Float wire covers signed zero and infinity; kind7 stores a uint64 value in the shared node uint64 field, not a pointer address. Existing patch validator independently rejects uint64 and nonfinite float inputs.',
  'Safety policy rejects empty/malformed input, empty keys, unsupported encodings, invalid fixed-width hex, oversized source/token/node/depth, top-level include/base directives, unquoted bracket conditions and alternate equals assignments. These refusals are intentional; malformed original error recovery is not reproduced.',
  'All87 referenced benchmark raw VMTs (86 configured loose files and1 BSP-packed patch) and2 additional packed VMTs matched. Reading raw declarations does not establish effective material parameters. Include resolution, filesystem/BSP precedence, condition evaluation, shader defaults/fallbacks and actual IMaterial production remain UNVERIFIED.',
  'Existing sources unchanged. No editor integration/build, renderer/animation/document/save/serialization changes, lighting-guard removal, dependency installs, original/session writes or UI/SFM/Blender sessions. Whole application remains PARTIALLY VERIFIED; complete workflow fidelity not demonstrated.'
 ]
 result.update(status='PASS',safety=p.stdout.strip(),executables=executables,build_logs=builds,changed_sources=changed,sources=sources,unchanged_prior_sources=len(prior['sources']),limitations=limits)
 receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2));(OUT/'source-integrity.json').write_text(json.dumps(dict(status='PASS',authoritative_receipt=str(receipt),receipt_sha256=sha(receipt),changed_sources=changed,sources=sources,executables=executables,build_logs=builds,original_hashes=originals),indent=2))
 lines=['VERIFIED original raw material KeyValues parser within checked subset.','Whole application PARTIALLY VERIFIED.',str(receipt),'Original/owned complete typed document comparisons='+str(len(cases))+' differences=0',p.stdout.strip(),'Unchanged original hashes='+str(len(originals)),'Unchanged prior source dependencies='+str(len(prior['sources'])),'','Exact freshly built/tested executables:']+[x['path']+' SHA256='+x['sha256'] for x in executables]+['','New sources:']+list(changed)+['','Limits:']+limits+['','Reproduce:','cmd /c work\\build_material_keyvalues_parser.cmd','C:\\Users\\wigwo\\AppData\\Local\\Python\\bin\\python3.14.exe work\\verify_material_keyvalues_parser.py']
 (OUT/'verification.txt').write_text('\n'.join(lines)+'\n');print('PASS '+str(receipt),flush=True)

if __name__=='__main__':main()
