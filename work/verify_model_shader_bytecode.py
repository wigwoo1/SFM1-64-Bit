"""Sample original model shader blocks without decoding every combo into memory."""
import hashlib,json,pathlib,struct,subprocess,uuid
from vcs6_shader_cache_candidate import Vcs6ShaderCache
root=pathlib.Path(__file__).resolve().parent.parent
source=pathlib.Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\platform\shaders\fxc')
out=root/'outputs'/('model-shader-bytecode-'+uuid.uuid4().hex);out.mkdir()
lines=[];results=[];failures=[]
paths=sorted(set(source.glob('vertexlit_and_unlit_generic*.vcs'))|set(source.glob('phong*.vcs'))|set(source.glob('skin*.vcs')))
for path in paths:
 data=path.read_bytes();version,total,dynamic=struct.unpack_from('<III',data);count=struct.unpack_from('<I',data,20)[0]
 if version!=6 or count<2 or count>(len(data)-28)//8:raise RuntimeError('unsupported index: '+path.name)
 entries=[struct.unpack_from('<II',data,28+i*8) for i in range(count)]
 checked=0;samples=[]
 for i in sorted({0,(count-2)//2,count-2}):
  key,start=entries[i];end=entries[i+1][1];payload=data[start:end]
  # Independent Python decoder receives the exact selected payload, in a
  # valid single-primary index. Original key/header/compression are retained.
  fixture=data[:20]+struct.pack('<II',2,struct.unpack_from('<I',data,24)[0])
  fixture+=struct.pack('<II',key,48)+struct.pack('<II',0xffffffff,48+len(payload))+struct.pack('<I',0)+payload
  try:oracle=Vcs6ShaderCache(fixture)
  except Exception as error:
   failures.append({'file':path.name,'static':key,'error':str(error),'token':hex(struct.unpack_from('<I',payload)[0])});continue
  records=oracle.payloads[key];ids=sorted(records)
  if not ids:raise RuntimeError('empty sample')
  for dynamicId in sorted({ids[0],ids[len(ids)//2],ids[-1]}):
   shader=records[dynamicId];expected=out/(hashlib.sha256(shader).hexdigest()+'.bin')
   if not expected.exists():expected.write_bytes(shader)
   for offset in sorted({0,dynamic-1}):
    lines.extend([str(path),f'{key*dynamic+offset} {dynamicId}',str(expected)]);checked+=1
  samples.append({'static':key,'payload_bytes':len(payload),'dynamic_records':len(records)})
 results.append({'file':str(path),'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'static_entries':count-1,'dynamic_combos':dynamic,'checks':checked,'samples':samples})
manifest=out/'oracle.txt';manifest.write_text('\n'.join(lines)+'\n')
run=subprocess.run([str(root/'work/test_vcs6_shader_bytecode.exe'),str(manifest)],capture_output=True,text=True)
for result in results:
 if hashlib.sha256(pathlib.Path(result['file']).read_bytes()).hexdigest()!=result['sha256']:raise RuntimeError('installed input changed')
report={'caches':results,'unsupported_samples':failures,'x64_exit_code':run.returncode,'x64_result':run.stdout.strip(),'x64_error':run.stderr.strip(),'installed_inputs_unchanged':True,'limits':'First/middle/last primary blocks and representative dynamic records only. Full model material combo selection, all cache blocks, native loader parity and rendering remain unverified.'}
(root/'outputs/sfm64-model-shader-bytecode-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({'files':len(results),'checks':sum(r['checks'] for r in results),'unsupported':failures,'result':run.stdout,'error':run.stderr,'exit':run.returncode},indent=2))
if run.returncode:raise SystemExit(run.returncode)
