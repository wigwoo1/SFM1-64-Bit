import hashlib,json,pathlib,struct,subprocess,uuid
from vcs6_shader_cache_candidate import Vcs6ShaderCache
root=pathlib.Path(__file__).resolve().parent.parent
out=root/'outputs'/('vcs6-index-'+uuid.uuid4().hex);out.mkdir()
source=pathlib.Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\platform\shaders\fxc')
lines=[];files=[];selections=0
for path in sorted(source.glob('unlitgeneric*.vcs')):
 data=path.read_bytes();cache=Vcs6ShaderCache(data);rows=[]
 for arg in range(cache.total_combos):
  normalized=arg//cache.dynamic_combos;resolved=cache.aliases.get(normalized,normalized)
  entry=next((i for i,pair in enumerate(cache.entries[:-1]) if pair[0]==resolved),None)
  if entry is None:continue
  for dyn in range(cache.dynamic_combos):
   rows.append(f'{arg} {dyn} {normalized} {resolved} {cache.entries[entry][1]} {cache.entries[entry+1][1]}')
 lines.extend([str(path),str(len(rows)),*rows]);selections+=len(rows)
 files.append({'path':str(path),'sha256':hashlib.sha256(data).hexdigest(),'selections':len(rows)})
fixture=(source/'unlitgeneric_vs20.vcs').read_bytes()
bad=[fixture[:n] for n in (0,4,27,39,len(fixture)-1)]
for offset,value in ((0,7),(8,0),(20,0),(32,0),(28,0xffffffff),(100,0xffffffff),(108,0xffffffff)):
 changed=bytearray(fixture);struct.pack_into('<I',changed,offset,value);bad.append(bytes(changed))
for i,data in enumerate(bad):
 try:Vcs6ShaderCache(data)
 except ValueError:pass
 else:raise RuntimeError('malformed fixture oracle accepted')
 path=out/f'bad-{i}.vcs';path.write_bytes(data);lines.extend([str(path),'0'])
manifest=out/'oracle.txt';manifest.write_text('\n'.join(lines)+'\n')
result=subprocess.run([str(root/'work/test_vcs6_shader_cache_index.exe'),str(manifest)],capture_output=True,text=True,check=True)
for file in files:
 if hashlib.sha256(pathlib.Path(file['path']).read_bytes()).hexdigest()!=file['sha256']:raise RuntimeError('installed input changed')
report={'files':files,'selections':selections,'malformed_index_rejections':len(bad),'installed_inputs_unchanged':True,'result':result.stdout.strip(),'limits':'Index selection only; dynamic record existence, LZMA decoding, original full loader execution and GPU shader binding remain separate and unverified in this C++ component.'}
(root/'outputs/sfm64-vcs6-index-validation.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
