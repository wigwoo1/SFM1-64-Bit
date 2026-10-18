from pathlib import Path
import subprocess, uuid, hashlib, json
root=Path(__file__).resolve().parent.parent
exe=root/'build/session-core/sfm64-session-core.exe'
directory=root/'outputs'/('float-key-cli-'+uuid.uuid4().hex)
directory.mkdir()
checks=[]
for fixture_id,layer in [(11,2814),(139,3199)]:
 fixture=root/f'work/roundtrip-{fixture_id}.dmx'
 original=fixture.read_bytes()
 def run(output,l,index,value):
  return subprocess.run([str(exe),'edit-float-key',str(fixture),str(output),str(l),str(index),value],capture_output=True,text=True)
 for index,value in [(0,'123'),(1,'456')]:
  output=directory/f'{fixture_id}-{index}.dmx'
  result=run(output,layer,index,value)
  assert result.returncode==0,result.stderr
  subprocess.run([str(root/'work/test_saved_float_key.exe'),str(fixture),str(output),str(layer),str(index),value],check=True)
  saved=output.read_bytes()
  assert run(output,layer,index,'789').returncode!=0 and output.read_bytes()==saved
 for i,(l,index,value) in enumerate([(layer,4294967295,'1'),(4294967295,0,'1'),(0,0,'1'),(layer,0,'1junk'),(layer,'-1','1')]):
  output=directory/f'{fixture_id}-invalid-{i}.dmx'
  assert run(output,l,index,value).returncode!=0 and not output.exists()
 assert fixture.read_bytes()==original
 checks.append({'fixture':fixture_id,'layer':layer,'saved_keys_verified':2,'full_graph_preserved':True,'overwrite_rejections':2,'invalid_rejections':5,'input_unchanged':True})
report={'checks':checks,'executable_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'output_directory':str(directory),'limits':'Serialized existing float key edits only; no GUI key editor, new keys, native scheduler or native renderer parity.'}
(root/'outputs/sfm64-float-key-cli-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print('float_key_cli_saved=4 overwrite_rejections=4 invalid_rejections=10 full_graph_preserved=true')
