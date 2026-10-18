"""Read-only original shader metadata evidence; not an effective-material implementation."""
import hashlib,json,re,shutil,struct,subprocess,tempfile
from pathlib import Path
from pe_image import Image
from verify_ifm_draw_packet import check_ranges
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/material-effective-parameters-20261017'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
PRIOR=ROOT/'outputs/material-patch-files-20261017/verification-zsdt1ra8/results.json'
DUMP=Path('C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/dumpbin.exe')
def require(ok,message):
 if not ok:raise RuntimeError(message)
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
class Wire:
 def __init__(self,raw):self.raw=raw;self.at=0
 def number(self,fmt='<I'):
  value=struct.unpack_from(fmt,self.raw,self.at)[0];self.at+=struct.calcsize(fmt);return value
 def text(self):
  n=self.number();require(n<=16384,'Wire text bound');v=self.raw[self.at:self.at+n].decode('ascii');self.at+=n;return v
 def end(self):require(self.at==len(self.raw),'Wire trailing/missing bytes')
def decode(raw):
 w=Wire(raw);result=[]
 for i in range(w.number()):
  obj,table,*methods=[w.number() for _ in range(6)];name=w.text();literal=bool(w.number());fallback_present=bool(w.number()) if literal else False;fallback=w.text() if fallback_present else None;params=[]
  for k in range(w.number()):
   rva=w.number();key=w.text();help=w.text();typ=w.number();default_present=bool(w.number());default=w.text() if default_present else None;flags=w.number();params.append(dict(rva=rva,name=key,help=help,type=typ,default=default,flags=flags))
  result.append(dict(index=i,object_rva=obj,vtable_rva=table,methods=methods,name=name,literal_fallback_verified=literal,literal_fallback=fallback,parameters=params))
 w.end();return result

def main():
 prior=json.loads(PRIOR.read_text());require(prior['status']=='PASS','Prior checkpoint');originals=dict(prior['original_hashes']);shader_path=GAME/'bin/stdshader_dx9.dll';originals[str(shader_path)]='fb09e2ddc8a99aff4ef31e3fddfdb1ca84d89379336abdab4e76e94adfa7e6f8'
 sources=json.loads((ROOT/'outputs/material-patch-files-20261017/source-integrity.json').read_text())['sources']
 for path,digest in sources.items():require(sha(path)==digest,'Unrelated prior source changed '+path)
 for path,digest in originals.items():require(sha(path)==digest,'Original identity '+path)
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True)
 # Fresh compile of this reference-only reader. No editor build is claimed.
 build=subprocess.run(['cmd','/c',str(ROOT/'work/build_shader_parameter_registry.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=90);(run/'build-driver.log').write_text(build.stdout+build.stderr);require(build.returncode==0,'Registry build failed');build_log=(OUT/'registry-build.log').read_text();(run/'registry-build.log').write_text(build_log);require(not re.search(r'\b(?:warning|error) C\d+',build_log),'Compiler diagnostics')
 exe=OUT/'shader_parameter_registry_reference_x86.exe';raw=exe.read_bytes();p=struct.unpack_from('<I',raw,60)[0];require(raw[p:p+4]==b'PE\0\0' and struct.unpack_from('<H',raw,p+4)[0]==0x14c and b'asInvoker' in raw,'Reference reader architecture/manifest')
 imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/'imports.txt').write_text(imports);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require(set(m.lower() for m in modules)=={'kernel32.dll'},'Unexpected direct imports')
 libs=run/'copied-libraries';libs.mkdir()
 for name in ['tier0.dll','stdshader_dx9.dll']:
  shutil.copyfile(GAME/'bin'/name,libs/name);require(sha(libs/name)==originals[str(GAME/'bin'/name)],'Copied identity '+name)
 # These actual instruction ranges establish the observed interface/metadata ABI.
 im,evidence=check_ranges(shader_path,'stdshader_dx9-disassembly.txt',[
 ('registry_count',0x10065c40,0x10065c44),('registry_get_and_bounds',0x10065c50,0x10065c6f),
 ('base_parameter_count',0x10064a00,0x10064a06),('base_parameter_info',0x10064a10,0x10064a24),
 ('base_no_fallback',0x10041370,0x10041375),('refract_literal_fallback',0x1003eea0,0x1003eea8),
 ('water_literal_fallback',0x1005caa0,0x1005caa8),('terrain_literal_fallback',0x10061ac0,0x10061ac8)])
 require([im.word(0x1009eae4+4*i) for i in [2,3]]==[0x10065c40,0x10065c50],'Registry vtable identity');(run/'original-code-evidence.json').write_text(json.dumps(evidence,indent=2))
 outputs=[]
 for i in range(2):
  out=run/('registry-'+str(i)+'.bin');p=subprocess.run([str(exe),str(libs),str(out)],cwd=run,capture_output=True,text=True,timeout=60);(run/('native-'+str(i)+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,'Original registry process exit '+str(p.returncode));outputs.append(out.read_bytes())
 require(outputs[0]==outputs[1],'Fresh-process registry metadata differs');records=decode(outputs[0]);require(len(records)==157,'Pinned registry count')
 names={r['name'].lower():r for r in records};require(len(names)==len(records),'Ambiguous case-folded shader identity');static_records=0;runtime_records=0;literal_count=0;conditional=[];accessors=[]
 for r in records:
  table=im.base+r['vtable_rva'];require([im.word(table+4*j)-im.base for j in range(4)]==r['methods'],'Live/static shader vtable mismatch')
  name_address=im.base+r['methods'][0];off=im.offset(name_address);name_code=im.b[off:off+6];require(name_code[0]==0xa1 and name_code[5]==0xc3,'Name accessor ABI');name_slot=struct.unpack_from('<I',name_code,1)[0];require(im.string(im.word(name_slot))==r['name'],'Static name mismatch')
  count_address=im.base+r['methods'][2];off=im.offset(count_address);count_code=im.b[off:off+16];require(count_code[:3]==b'\x56\x8b\x35' and count_code[7]==0xe8 and count_code[12:]==b'\x03\xc6\x5e\xc3','Count accessor ABI');require(count_address+12+struct.unpack_from('<i',count_code,8)[0]==0x10064a00,'Count base method target')
  accessors.append(dict(shader=r['name'],name_accessor_va=hex(name_address),name_bytes=name_code.hex(),count_accessor_va=hex(count_address),count_bytes=count_code.hex(),note='Count adds normal CRT-initialized extra-parameter count to base13; raw disk extra count is not the initialized count.'))
  off=im.offset(im.base+r['methods'][1]);code=im.b[off:off+8];literal=code[0]==0xb8 and code[5:8]==b'\xc2\x04\x00';null=code[:5]==b'\x33\xc0\xc2\x04\x00';require(r['literal_fallback_verified']==(literal or null),'Fallback whitelist mismatch')
  if literal or null:
   expected=im.string(struct.unpack_from('<I',code,1)[0]) if literal else None;require(r['literal_fallback']==expected,'Original literal fallback mismatch');literal_count+=1
   if expected is not None:require(expected.lower() in names,'Literal fallback target absent')
  else:conditional.append(r['name'])
  for k,param in enumerate(r['parameters']):
   va=im.base+param['rva']
   if k<13:
    require(va==0x100bce28+20*k,'Base13 descriptor identity '+r['name']);v=[im.word(va+j*4) for j in range(5)];static=dict(rva=param['rva'],name=im.string(v[0]),help=im.string(v[1]) if v[1] else '',type=v[2],default=im.string(v[3]) if v[3] else None,flags=v[4]);require(param==static,'Original/static base parameter mismatch '+r['name']+' '+str(k));static_records+=1
   else:
    # Extra descriptor storage is constructed by the DLL CRT. Its raw disk bytes
    # may be zero; these are actual initialized runtime records, not static data.
    require(param['name'],'Empty runtime parameter name');runtime_records+=1
 require(static_records==2041 and runtime_records==1483,'Pinned base/constructed parameter coverage');(run/'metadata-accessor-evidence.json').write_text(json.dumps(accessors,indent=2));surface=[(r['name'],p['name']) for r in records for p in r['parameters'] if p['name'].lower() in ['$surfaceprop','$surfaceprop2']];require(not surface,'Surface parameter declarations unexpectedly present')
 # Relate metadata to the already verified raw benchmark documents. This is
 # deliberately not an effective-material comparison or a new original load.
 previous_run=Path(prior['path']);actual=next(s for s in prior['scenarios'] if s['name']=='actual-benchmark-materials');data=previous_run/'actual-benchmark-materials-native.bin';request=previous_run/'actual-benchmark-materials-input.bin';require(sha(data)==actual['output_sha256'] and sha(request)==actual['input_sha256'],'Prior raw benchmark provenance')
 wi=Wire(request.read_bytes())
 for _ in range(wi.number()):wi.text();wi.text();wi.number()
 material_names=[(wi.text(),wi.number()) for _ in range(wi.number())];wi.end();w=Wire(data.read_bytes())
 def node():
  key=w.text();kind=w.number();v=w.text() if kind==1 else w.number() if kind in [2,3] else w.number('<Q') if kind in [4,7] else None;return dict(key=key,kind=kind,value=v,children=[node() for _ in range(w.number())])
 benchmark=[]
 for name,mode in material_names:
  if not benchmark:require(w.number()==89,'Prior actual benchmark count')
  require(w.number()==1,'Prior material loader failure');roots=[node() for _ in range(w.number())];node();require(roots,'Missing prior raw root');shader=roots[0]['key'];entry=names.get(shader.lower());chain=[];seen=set();unresolved=None
  while entry is not None:
   if entry['name'].lower() in seen:unresolved='literal fallback cycle';break
   seen.add(entry['name'].lower());chain.append(dict(shader=entry['name'],declared_parameter_count=len(entry['parameters'])))
   if not entry['literal_fallback_verified']:unresolved='conditional fallback not executed';break
   target=entry['literal_fallback']
   if target is None:break
   entry=names.get(target.lower())
  if not chain:unresolved='shader absent from this DLL registry'
  benchmark.append(dict(material=name,raw_shader=shader,literal_metadata_chain=chain,unresolved_fallback=unresolved,raw_surface_declarations=[c for c in roots[0]['children'] if c['key'].lower() in ['$surfaceprop','$surfaceprop2']]))
 w.end()
 # Argument validation is an additional reader check, not an editor/UI test.
 bad=subprocess.run([str(exe)],cwd=run,capture_output=True,text=True,timeout=15);(run/'bad-arguments.log').write_text(bad.stdout+bad.stderr);require(bad.returncode==1,'Reader arguments not refused')
 for path,digest in originals.items():require(sha(path)==digest,'Original changed after reads '+path)
 for path,digest in sources.items():require(sha(path)==digest,'Prior implementation changed '+path)
 (run/'shader-metadata.json').write_text(json.dumps(records,indent=2));(run/'benchmark-metadata-links.json').write_text(json.dumps(benchmark,indent=2))
 limitations=['No effective IMaterial construction/FindVar comparison yet. Raw VMT declarations are not published as effective parameters.','No shader InitParams/texture loading/render execution/GPU initialization. Declared defaults can be changed by later initialization.','Conditional or hardware-dependent fallback methods are not executed. Only exact literal-return methods are verified.','This x86 program is a reference-only test oracle, never a dependency of the owned x64 editor. No editor integration or rebuild in this milestone.']
 result=dict(status='PASS',scope='original shader registry metadata and literal-return fallback evidence only',path=str(run),shaders=len(records),parameter_records=static_records+runtime_records,static_base_record_comparisons=static_records,crt_initialized_extra_records=runtime_records,observed_raw_parameter_type_codes=sorted({p['type'] for r in records for p in r['parameters']}),process_runs=2,byte_identical_runs=True,literal_fallback_methods=literal_count,unexecuted_conditional_fallback_methods=conditional,surface_parameter_declarations=surface,prior_benchmark_material_documents=len(benchmark),benchmark_raw_shader_counts={name:sum(b['raw_shader'].lower()==name for b in benchmark) for name in sorted({b['raw_shader'].lower() for b in benchmark})},benchmark_unresolved_fallbacks=[b['material'] for b in benchmark if b['unresolved_fallback']],original_hashes=originals,unverified=limitations,executable=dict(path=str(exe),sha256=sha(exe),machine='0x14c',direct_imports=modules),output_sha256=hashlib.sha256(outputs[0]).hexdigest())
 (run/'results.json').write_text(json.dumps(result,indent=2));newfiles=['work/probe_shader_parameter_registry.cpp','work/build_shader_parameter_registry.cmd','work/verify_shader_parameter_registry.py'];integrity=dict(status='PASS',authoritative_receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json'),new_sources={str(ROOT/f):sha(ROOT/f) for f in newfiles},unchanged_prior_sources=sources,executable=result['executable']);(OUT/'source-integrity.json').write_text(json.dumps(integrity,indent=2))
 report='VERIFIED original shader metadata evidence; application remains PARTIALLY VERIFIED.\n'+f"157 actual registered shaders; 3524 actual metadata records (2041 base records matched static PE declarations; 1483 extra records constructed by original DLL initialization); 2 fresh processes byte-identical; {literal_count} proven literal fallback methods; {len(conditional)} conditional methods unexecuted.\nNo shader declarations of $surfaceprop/$surfaceprop2 were found in this DLL registry. This does not prove their effective material values or absence.\n"+f"89 prior verified benchmark raw documents linked to metadata; {len(result['benchmark_unresolved_fallbacks'])} unresolved metadata fallback chains.\n"+'\n'.join(limitations)+'\nExecutable tested: '+str(exe)+'\nOriginal file hashes unchanged: '+str(len(originals))+'\nReceipt: '+str(run/'results.json')+'\n'
 (run/'verification.txt').write_text(report);(OUT/'verification.txt').write_text(report);print(report)
if __name__=='__main__':main()
