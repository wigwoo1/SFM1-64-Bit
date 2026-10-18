from pathlib import Path
import subprocess,re,json,hashlib
s=Path('work/test_vtf73_texture.cpp').read_text().replace('argc!=2','argc!=3').replace('auto bad=b;', 'std::ofstream output(argv[2],std::ios::binary);output.write(reinterpret_cast<const char*>(image.rgba.data()),image.rgba.size());output.close();if(!output)throw std::runtime_error("iris pixel export failed");auto bad=b;')
Path('work/test_iris_texture.cpp').write_text(s)
s=Path('work/build_vtf73_texture_test.cmd').read_text().replace('test_vtf73_texture','test_iris_texture');Path('work/build_iris_texture_test.cmd').write_text(s)
r=subprocess.run(['cmd.exe','/d','/c','work\\build_iris_texture_test.cmd'],capture_output=True,text=True);assert r.returncode==0,r.stdout+r.stderr
records=[]
for index in [2,4]:
 log=Path(f'outputs/sfm64-alternate-render-inputs-{index}.txt').read_text()
 for n,path in enumerate(re.findall(r'iris_resource=(.*?) shader=',log)):
  output=Path(f'outputs/iris-{index}-{n}-decoded.rgba');r=subprocess.run(['build/session-core/test_iris_texture.exe',path,str(output)],capture_output=True,text=True)
  records.append({'asset_index':index,'texture':path,'passed':r.returncode==0,'failure':r.stderr.strip(),'pixel_sha256':hashlib.sha256(output.read_bytes()).hexdigest() if r.returncode==0 else None})
Path('outputs/sfm64-iris-texture-pixel-validation.json').write_text(json.dumps(records,indent=2)+'\n');print(json.dumps(records,indent=2))
