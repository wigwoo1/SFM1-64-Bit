import hashlib,json,pathlib,struct,subprocess,uuid
from vcs6_shader_cache_candidate import Vcs6ShaderCache
root=pathlib.Path(__file__).resolve().parent.parent
out=root/'outputs'/('vcs6-bytecode-'+uuid.uuid4().hex);out.mkdir()
source=pathlib.Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\platform\shaders\fxc')
lines=[];inputs={};checks=0;rejections=0
for path in sorted(source.glob('unlitgeneric*.vcs')):
 data=path.read_bytes();inputs[str(path)]=hashlib.sha256(data).hexdigest();cache=Vcs6ShaderCache(data)
 for static in list(cache.payloads)+list(cache.aliases):
  for dynamic,shader in cache.payloads[cache.aliases.get(static,static)].items():
   expected=out/(hashlib.sha256(shader).hexdigest()+'.bin')
   if not expected.exists():expected.write_bytes(shader)
   for offset in range(cache.dynamic_combos):
    lines.extend([str(path),f'{static*cache.dynamic_combos+offset} {dynamic}',str(expected)]);checks+=1
 # Missing dynamic variants must fail rather than silently choose a neighbor.
 for static,records in cache.payloads.items():
  for dynamic in range(cache.dynamic_combos):
   if dynamic not in records:lines.extend([str(path),f'{static*cache.dynamic_combos} {dynamic}','REJECT']);rejections+=1
fixture=(source/'unlitgeneric_vs20.vcs').read_bytes()
for i,(offset,value) in enumerate(((168,0x80000001),(172,0),(176,0xffffffff),(180,0xffffffff),(185,0xffffffff))):
 bad=bytearray(fixture);struct.pack_into('<I',bad,offset,value);path=out/f'bad-{i}.vcs';path.write_bytes(bad)
 lines.extend([str(path),'0 0','REJECT']);rejections+=1
manifest=out/'oracle.txt';manifest.write_text('\n'.join(lines)+'\n')
result=subprocess.run([str(root/'work/test_vcs6_shader_bytecode.exe'),str(manifest)],capture_output=True,text=True,check=True)
for path,digest in inputs.items():
 if hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()!=digest:raise RuntimeError('installed cache changed')
dependency=root/'work/dependencies/lzma-decoder'
report={'byte_exact_selector_checks':checks,'missing_or_malformed_rejections':rejections,'installed_inputs_unchanged':True,'cache_hashes':inputs,'decoder_upstream_commit':(dependency/'UPSTREAM_COMMIT.txt').read_text().strip(),'decoder_source_hashes':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in dependency.glob('*') if p.is_file()},'result':result.stdout.strip(),'limits':'Observed VCS6 LZMA blocks only. Retained cache shares resolved static blocks, enforces a bytecode budget and serializes cold decodes; metadata overhead is additional. Native full loader parity, application renderer integration and GPU binding remain unfinished. Declared output length is enforced without requiring EOS; trailing range-coder byte policy is not established.'}
(root/'outputs/sfm64-vcs6-bytecode-validation.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
