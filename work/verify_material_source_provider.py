"""Actual owned loose/BSP sources vs normally loaded original filesystem.
ZIP layout: PKWARE APPNOTE6.3.10 sections4.3.7,4.3.12,4.3.16.
https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT
"""
import hashlib,io,json,re,shutil,struct,subprocess,tempfile,zipfile
from pathlib import Path
from verify_material_keyvalues_parser import text,decode
from verify_ifm_draw_packet import check_ranges
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/material-source-provider-20261017'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
PRIOR=ROOT/'outputs/material-keyvalues-files-20261017/verification-j6da5x2q/results.json'
DUMP=Path('C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/dumpbin.exe')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
 if not ok:raise RuntimeError(message)
def request(mounts,cases):return struct.pack('<I',len(mounts))+b''.join(text(str(p))+text(id)+struct.pack('<I',position) for p,id,position in mounts)+struct.pack('<I',len(cases))+b''.join(text(path)+text(id) for path,id in cases)
def read_request(raw):
 at=0
 def word():
  nonlocal at
  v=struct.unpack_from('<I',raw,at)[0];at+=4;return v
 def string():
  nonlocal at
  n=word();s=raw[at:at+n].decode('ascii');at+=n;return s
 mounts=[(Path(string()),string(),word()) for _ in range(word())];cases=[(string(),string()) for _ in range(word())];require(at==len(raw),'Request wire span');return mounts,cases

def byte_decode(raw):
 at=4;out=[]
 for _ in range(struct.unpack_from('<I',raw)[0]):
  found,size=struct.unpack_from('<II',raw,at);at+=8;require(found in [0,1] and (found or not size) and size<=len(raw)-at,'Byte result span');out.append(raw[at:at+size] if found else None);at+=size
 require(at==len(raw),'Byte result trailing span');return out

def zip_bytes(entries,extra=b'',comment=b''):
 b=io.BytesIO()
 with zipfile.ZipFile(b,'w',compression=zipfile.ZIP_STORED) as archive:
  archive.comment=comment
  for name,data in entries:
   info=zipfile.ZipInfo(name);info.create_system=0;info.extra=extra;info.external_attr=16 if name.endswith('/') else 0;archive.writestr(info,data)
 return b.getvalue()
def bsp_bytes(z):
 header=bytearray(1036);header[:4]=b'VBSP';struct.pack_into('<I',header,4,20);struct.pack_into('<IIII',header,8+40*16,1036,len(z),0,0);return bytes(header)+z

def fixtures(root):
 root.mkdir();(root/'test-only.marker').write_text('Disposable source-provider verification fixtures only.');loose=root/'loose';loose.mkdir();(loose/'extras').mkdir();(loose/'extras/extra.vmt').write_bytes(b'Extra{from loose}');(loose/'materials').mkdir();(loose/'materials/loose-root.vmt').write_bytes(b'#base base.vmt Loose{local loose}');(loose/'shared').mkdir();(loose/'shared/value.bin').write_bytes(b'from-loose')
 outside=root/'outside';outside.mkdir();(outside/'redirect.vmt').write_bytes(b'Shader{redirected 1}');junction=loose/'redirect';link=subprocess.run(['cmd','/c','mklink','/J',str(junction),str(outside)],capture_output=True,text=True);
 if link.returncode:(root/'reparse-fixture-unavailable.txt').write_text(link.stdout+link.stderr)
 with (loose/'oversized.bin').open('wb') as f:f.seek((1<<24));f.write(b'x')
 entries=[('materials/root.vmt',b'Shader{local frompak} #base base.vmt #include ../extras/extra.vmt'),('materials/base.vmt',b'Base{$surfaceprop metal}'),('data.bin',bytes(range(256))),('zero.bin',b''),('shared/value.bin',b'from-pack'),('Materials/Mixed.VMT',b'Shader{from mixed}'),('folder/',b'')];z=zip_bytes(entries,comment=b'portable source provider');(root/'good.zip').write_bytes(z);(root/'good.bsp').write_bytes(bsp_bytes(z));(root/'owned-copy.bsp').write_bytes(bsp_bytes(z));(root/'empty.bsp').write_bytes(bsp_bytes(zip_bytes([])));(root/'no-pak.bsp').write_bytes(bsp_bytes(b''));(root/'extra.bsp').write_bytes(bsp_bytes(zip_bytes(entries,extra=struct.pack('<HHBI',0x5455,5,1,123))));utf=bytearray(z);c=struct.unpack_from('<I',z,z.rfind(b'PK\x05\x06')+16)[0]
 for _ in entries:
  local=struct.unpack_from('<I',utf,c+42)[0];struct.pack_into('<H',utf,c+8,0x800);struct.pack_into('<H',utf,local+6,0x800);nl,el,cl=struct.unpack_from('<HHH',utf,c+28);c+=46+nl+el+cl
 (root/'utf-flag.bsp').write_bytes(bsp_bytes(utf));(root/'not-directory.txt').write_text('not a directory');(root/'implicit').mkdir();(root/'implicit/pak01.vpk').write_bytes(b'unsupported')
 end=z.rfind(b'PK\x05\x06');central=struct.unpack_from('<I',z,end+16)[0];badCount=0
 def bad(name,raw,is_bsp=False):
  nonlocal badCount
  (root/('bad-'+name+'.bsp')).write_bytes(bytes(raw) if is_bsp else bsp_bytes(bytes(raw)));badCount+=1
 def mutate(name,where,fmt,value):raw=bytearray(z);struct.pack_into(fmt,raw,where,value);bad(name,raw)
 for name,off,fmt,value in [('end-disk',end+4,'<H',1),('end-central-disk',end+6,'<H',1),('end-count',end+8,'<H',0),('end-total',end+10,'<H',1),('end-zip64',end+10,'<H',65535),('central-size',end+12,'<I',0),('central-offset',end+16,'<I',0xffffffff),('central-signature',central,'<I',0),('local-signature',0,'<I',0),('local-offset',central+42,'<I',central),('crc-central',central+16,'<I',0),('crc-local',14,'<I',0),('compressed-size',central+20,'<I',0),('uncompressed-size',central+24,'<I',0x1000001),('name-size',central+28,'<H',65535),('extra-size',central+30,'<H',65535),('comment-size',central+32,'<H',65535),('entry-disk',central+34,'<H',1),('file-symlink',central+38,'<I',0xa1ff0000),('needed-version',central+6,'<H',45),('central-needed-size',central+24,'<I',0xffffffff),('local-name-size',26,'<H',65535),('local-extra-size',28,'<H',65535)]:mutate(name,off,fmt,value)
 for name,flags,method in [('compression',0,8),('encryption',1,0),('descriptor',8,0),('reserved-flag',0x4000,0)]:
  raw=bytearray(z);struct.pack_into('<H',raw,central+8,flags);struct.pack_into('<H',raw,6,flags);struct.pack_into('<H',raw,central+10,method);struct.pack_into('<H',raw,8,method);bad(name,raw)
 bad('truncated-end',z[:-1]);bad('missing-end',z[:end]);raw=bytearray(z);raw[30]^=1;bad('local-name-mismatch',raw);raw=bytearray(z);start=30+len(entries[0][0]);raw[start]^=1;bad('file-data-crc',raw)
 for i,name in enumerate(['../escape','a/../b','a//b','/abs','C:/a','a\\b']):bad('unsafe-name-'+str(i),zip_bytes([(name,b'data')]) if '\\' not in name else zip_bytes([('a/b',b'data')]).replace(b'a/b',b'a\\b'))
 bad('duplicate-case',zip_bytes([('same.bin',b'a'),('SAME.BIN',b'b')]));bad('directory-data',zip_bytes([('directory/',b'data')]));bad('zip64-extra',zip_bytes([('a.bin',b'data')],extra=struct.pack('<HHQ',1,8,4)));bad('malformed-extra',zip_bytes([('a.bin',b'data')],extra=struct.pack('<HH',0x5455,100)));bad('unicode-override',zip_bytes([('a.bin',b'data')],extra=struct.pack('<HH',0x7075,0)))
 # A valid embedded local record aliases another file's payload; CRCs remain
 # valid, but the indexed local record spans overlap and must be refused.
 innerZip=zip_bytes([('inner.bin',b'hello')]);innerEnd=innerZip.rfind(b'PK\x05\x06');innerLocal=innerZip[:struct.unpack_from('<I',innerZip,innerEnd+16)[0]];overlap=bytearray(zip_bytes([('outer.bin',innerLocal),('inner.bin',b'hello')]));e=overlap.rfind(b'PK\x05\x06');c=struct.unpack_from('<I',overlap,e+16)[0];c2=c+46+len('outer.bin');struct.pack_into('<I',overlap,c2+42,30+len('outer.bin'));bad('overlapping-local-spans',overlap)
 for name,off,val in [('magic',0,0),('version',4,19),('lump-offset',8+40*16,0xffffffff),('lump-size',8+40*16+4,0xffffffff),('lump-version',8+40*16+8,1),('lump-compression',8+40*16+12,1)]:raw=bytearray(bsp_bytes(z));struct.pack_into('<I',raw,off,val);bad('bsp-'+name,raw,True)
 bad('bsp-short',b'VBSP',True);return entries,badCount

def main():
 prior=json.loads(PRIOR.read_text());require(prior['status']=='PASS','Prior checkpoint');originals=dict(prior['original_hashes'])
 for path,digest in originals.items():require(sha(Path(path))==digest,'Original identity '+path)
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True);libs=run/'copied-libraries';libs.mkdir()
 for name in ['tier0.dll','vstdlib.dll','materialsystem.dll','filesystem_stdio.dll']:shutil.copyfile(GAME/'bin'/name,libs/name);require(sha(libs/name)==originals[str(GAME/'bin'/name)],'Copied library identity')
 _,evidence=check_ranges(GAME/'bin/filesystem_stdio.dll','filesystem-disassembly.txt',[('AddSearchPath',0x1000ea40,0x1000eae9)]);(run/'filesystem-code-evidence.json').write_text(json.dumps(evidence,indent=2));executables=[]
 for name,machine in [('source_native_x86',0x14c),('files_native_x86',0x14c),('source_owned_x64',0x8664),('source_resolved_x64',0x8664),('source_safety_x64',0x8664)]:
  exe=OUT/(name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine and b'asInvoker' in binary,'Executable architecture/manifest');imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(name+'-imports.txt')).write_text(imports);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require(modules and set(x.lower() for x in modules)=={'kernel32.dll'},'Unexpected direct dependency');executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules))
 def execute(name,args,label):
  p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=120);log=run/(label+'.log');log.write_text(p.stdout+p.stderr);require(p.returncode==0,'Program '+name+' exit '+str(p.returncode)+'; see '+str(log));require('KeyValues Error:' not in p.stdout+p.stderr,'Original parser diagnostics '+str(log));return p.stdout.strip()
 scenarios=[]
 def compare(label,mounts,cases,trees=False,expected=None,frozen=None):
  inp=run/(label+'-input.bin');inp.write_bytes(request(mounts,cases));native=run/(label+'-native.bin');owned=run/(label+'-owned.bin');execute('files_native_x86' if trees else 'source_native_x86',[libs,inp,native],label+'-native');execute('source_resolved_x64' if trees else 'source_owned_x64',[inp,owned],label+'-owned');require(native.read_bytes()==owned.read_bytes(),'Source mismatch '+label+'; see '+str(run));values=decode(owned.read_bytes()) if trees else byte_decode(owned.read_bytes())
  if frozen:require(owned.read_bytes()==frozen.read_bytes(),'Frozen tree result changed '+label)
  if expected is not None:require(values==expected,'Independent source byte expectation '+label)
  scenarios.append(dict(name=label,kind='resolved' if trees else 'bytes',cases=len(cases),input_sha256=sha(inp),output_sha256=sha(owned)));print('PASS '+label+' cases='+str(len(cases)),flush=True)
 oldRun=Path(prior['path']);controlled=run/'prior-fixtures';controlled.mkdir();shutil.copytree(oldRun/'high',controlled/'high');shutil.copytree(oldRun/'low',controlled/'low')
 for scenario in prior['scenarios']:
  name=scenario['name'];oldInput=oldRun/(name+'-native-input.bin');require(sha(oldInput)==scenario['native_input_sha256'],'Pinned request');mounts,cases=read_request(oldInput.read_bytes());mounts=[(controlled/p.name if p in [oldRun/'high',oldRun/'low'] else p,id,pos) for p,id,pos in mounts];compare(name+'-bytes',mounts,cases);compare(name+'-resolved',mounts,cases,True,frozen=oldRun/(name+'-native.bin'))
 bsp=GAME/'tf_movies/maps/mtt_soldier.bsp';raw=bsp.read_bytes();off,size=struct.unpack_from('<II',raw,8+40*16);archive=zipfile.ZipFile(io.BytesIO(raw[off:off+size]));names=[e.filename for e in archive.infolist()];variants=names+[names[0].upper(),names[1].replace('/','\\'),'materials/absent.vmt'];compare('all-original-packed-bytes',[(bsp,'GAME',1)],[(p,'GAME') for p in variants],expected=[archive.read(p) for p in names]+[archive.read(names[0]),archive.read(names[1]),None])
 fixtureRoot=run/'fixtures';entries,badCount=fixtures(fixtureRoot);valid=[(n,b) for n,b in entries if not n.endswith('/')];cases=[(n,'GAME') for n,b in valid]+[('absent','GAME')];expected=[b for n,b in valid]+[None]
 for name in ['good','extra','utf-flag']:compare('controlled-'+name+'-bytes',[(fixtureRoot/(name+'.bsp'),'GAME',1)],cases,expected=expected)
 for name in ['empty','no-pak']:compare('controlled-'+name,[(fixtureRoot/(name+'.bsp'),'GAME',1),(fixtureRoot/'loose','GAME',1)],[('zero.bin','GAME'),('shared/value.bin','GAME')],expected=[None,b'from-loose'])
 mounts=[(fixtureRoot/'good.bsp','GAME',1),(fixtureRoot/'loose','GAME',1)];compare('real-cross-pak-loose-graphs',mounts,[('materials/root.vmt','GAME'),('materials/loose-root.vmt','GAME'),('materials/mixed.vmt','GAME')],True)
 compare('controlled-pack-priority',mounts,[('shared/value.bin','GAME'),('zero.bin','GAME'),('data.bin','OTHER')],expected=[b'from-pack',b'',None]);compare('controlled-loose-priority',list(reversed(mounts)),[('shared/value.bin','GAME')],expected=[b'from-loose']);compare('controlled-head-priority',[(fixtureRoot/'loose','GAME',1),(fixtureRoot/'good.bsp','GAME',0)],[('shared/value.bin','GAME')],expected=[b'from-pack'])
 safety=execute('source_safety_x64',[fixtureRoot],'safety');print(safety,flush=True)
 for path,digest in originals.items():require(sha(Path(path))==digest,'Original changed '+path)
 for path,digest in prior['sources'].items():require(sha(Path(path))==digest,'Previous source changed '+path)
 result=dict(status='PASS_COMPARE',path=str(run),scenarios=scenarios,byte_cases=sum(x['cases'] for x in scenarios if x['kind']=='bytes'),resolved_cases=sum(x['cases'] for x in scenarios if x['kind']=='resolved'),safety=safety,unverified_checks=['Child reparse rejection: Windows denied disposable junction creation'] if (fixtureRoot/'reparse-fixture-unavailable.txt').exists() else [],malformed_archives=badCount,executables=executables,original_hashes=originals,reference_receipts={str(PRIOR):sha(PRIOR)},zip_specification='https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT')
 (run/'comparison.json').write_text(json.dumps(result,indent=2));print('PASS_COMPARE '+str(run/'comparison.json'),flush=True)
 builds=[]
 for name in ['native','files-native','owned','resolved','safety']:
  log=OUT/(name+'-build.log');require(not re.search(r'\b(?:warning|error) [A-Z]+\d+',log.read_text()),'Compiler diagnostic '+str(log));builds.append(dict(path=str(log),sha256=sha(log),warnings=0,errors=0))
 names=['source_stored_zip_candidate.h','source_material_source_provider_candidate.h','probe_material_source_native.cpp','probe_material_source_owned.cpp','test_material_source_provider.cpp','build_material_source_provider.cmd','verify_material_source_provider.py'];changed={str(ROOT/'work'/n):sha(ROOT/'work'/n) for n in names};sources=dict(changed);pending=[ROOT/'work'/n for n in names]+[ROOT/'work/probe_material_keyvalues_files_native.cpp'];visited=set()
 while pending:
  path=pending.pop()
  if path in visited:continue
  visited.add(path);sources[str(path)]=sha(path)
  if path.suffix in ['.cpp','.h']:
   for local in re.findall(r'^\s*#include\s*"([^"]+)"',path.read_text(),re.M):
    child=path.parent/local
    if child.is_file():pending.append(child)
 limits=[
  'Owned native64 provider reads real files and BSP pak bytes directly, without Python/catalog/native runtime dependencies. ZIP32 stored data are checked for EOCD/central/local spans, identities, uncompressed sizes, CRC32, unique normalized ASCII names, file kinds and non-overlapping/indexed local records. Archive snapshots immutable and owned, no extraction/writes. Raw source read limit16MiB, archive64MiB,128 explicit mounts; BSP20/uncompressed lump40 checked. These bounds/format refusals are explicit, not all SFM assets supported.',
  'Original byte oracle normally loads workspace copies of tier0/vstdlib/materialsystem/filesystem_stdio; actual VFileSystem017/IBaseFileSystem Open/Size/Read/Close and mount calls. Separate freshly compiled original KeyValues LoadFromFile oracle compares actual resolved trees. Original executable sections unchanged; no patches/private PE/fake filesystem/GPU/editor startup. Reference x86 programs are test oracles, not owned application dependencies.',
  '755 prior file/graph cases are re-read from copied disposable fixtures/original read-only assets and match both current native bytes and frozen native resolved trees; three additional graphs cross between real owned BSP and loose sources. Original17 packed entries including binary VTFs independently match Python zipfile bytes; uppercase/backslash aliases and missing-file distinction checked. Controlled stored archives cover binary/empty/directory records,UTF8 flag with ASCII names, timestamp extras, comments, empty ZIP/no-pak, and head/tail/path-ID/loose/archive priority. Does not prove rendered texture or material fidelity.',
  'Mounts are explicitly ordered. Test adapters apply original observed head/tail requests; provider filters explicit path IDs. Implicit directory ZIP/VPK/PAK mounts, compressed/encrypted/described/ZIP64/multi-disk/special/ambiguous ZIPs, non-ASCII logical names, duplicate mounts and non-BSP regular-file mounts explicitly refuse. No decompression/VPK/automatic Steam search graph or shader behavior guessed.',
  'Loose files read afresh with bounded size/checked regular-file and child reparse rejection. Component attributes used instead of canonical(), which was denied on otherwise readable F: folders. Child reparse rejection remains UNVERIFIED at runtime if Windows denies junction fixture creation; the guard remains present and denial is recorded. This is not an atomic/hostile-filesystem snapshot or proof against concurrent path replacement. BSP snapshots survive source-file/provider teardown; callback owns provider state. Safety writes only a marked disposable owned-copy.bsp, never original data.',
  '50 malformed archive fixtures,64 separate payload CRC corruptions, mount/path/extent/ownership/publication/concurrent checks leave prior published provider intact. Malformed archives are tested only against owned code, not fed to original DLLs. All original hashes and immediate previous source dependencies unchanged.',
  'This completes the explicit read-only loose/stored-BSP source provider and connects it to verified raw include/base parsing. Plain material patch include chains, shader defaults/fallbacks/conditions/effective IMaterial values, remaining full world trace fields and combined world/prop scene proof remain incomplete. Existing multiple-include lifetime/checked-parser refusals unchanged.',
  'No existing module changes/editor integration/full editor build, lighting-guard removal, animation/render/document/save/serialization changes, dependencies installed, original/session writes or SFM/Blender/UI sessions. Whole application PARTIALLY VERIFIED; complete original workflow not demonstrated.'
 ]
 result.update(status='PASS',build_logs=builds,changed_sources=changed,sources=sources,unchanged_prior_sources=len(prior['sources']),limitations=limits)
 receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2));(OUT/'source-integrity.json').write_text(json.dumps(dict(status='PASS',authoritative_receipt=str(receipt),receipt_sha256=sha(receipt),changed_sources=changed,sources=sources,executables=executables,build_logs=builds,original_hashes=originals),indent=2))
 lines=['VERIFIED owned native64 read-only explicit loose/stored-BSP source provider.','Whole application PARTIALLY VERIFIED.',str(receipt),'Native/owned byte read comparisons='+str(result['byte_cases'])+'; resolved document comparisons='+str(result['resolved_cases'])+'; differences=0',safety,'Unchanged original hashes='+str(len(originals)),'Unchanged immediate prior source dependencies='+str(len(prior['sources'])),'','Exact freshly built/tested executables:']+[x['path']+' SHA256='+x['sha256'] for x in executables]+['','New sources:']+list(changed)+['','Limits:']+limits+['','ZIP layout reference: https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT','','Reproduce:','cmd /c work\\build_material_source_provider.cmd','C:\\Users\\wigwo\\AppData\\Local\\Python\\bin\\python3.14.exe work\\verify_material_source_provider.py'];(OUT/'verification.txt').write_text('\n'.join(lines)+'\n');print('PASS '+str(receipt),flush=True)

if __name__=='__main__':main()
