from pathlib import Path
import sys,hashlib,uuid,subprocess,json
sys.path.insert(0,'work')
from pe_image import Image
p=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');im=Image(p);sha=hashlib.sha256(im.b).hexdigest();assert sha=='8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4'
rows=[]
for name,va,size in [('setter',0x103905c0,21),('getter',0x103905b0,9)]:
 o=im.offset(va);rows.append('inline constexpr unsigned char '+name+'[]={'+','.join(hex(b) for b in im.b[o:o+size])+'};')
Path('work/document_propagation_flag_bytes.h').write_text('#pragma once\n'+'\n'.join(rows))
for arch in ['32','64']:
 command='@echo off\ncall "C:\\Program Files (x86)\\Microsoft Visual Studio\\18\\BuildTools\\VC\\Auxiliary\\Build\\vcvars'+arch+'.bat" >nul\nif errorlevel 1 exit /b 1\ncl /nologo /std:c++17 /EHsc /W4 /O2 work\\probe_document_propagation_flag.cpp /Fowork\\probe_propagation_flag_'+arch+'.obj /Fework\\probe_propagation_flag_'+arch+'.exe\nexit /b %errorlevel%\n'
 script=Path('work/build_propagation_flag_'+arch+'.cmd');script.write_text(command);subprocess.run(['cmd','/c',str(script)],check=True)
oracle=Path('outputs/propagation-flag-'+uuid.uuid4().hex+'.bin')
a=subprocess.run(['work/probe_propagation_flag_32.exe',str(oracle)],check=True,capture_output=True,text=True);assert oracle.stat().st_size==65536*16
b=subprocess.run(['work/probe_propagation_flag_64.exe',str(oracle)],check=True,capture_output=True,text=True);assert 'native_propagation_flag_matches=65536' in b.stdout
assert hashlib.sha256(p.read_bytes()).hexdigest()==sha
report={'cases':65536,'native':a.stdout.strip(),'x64':b.stdout.strip(),'oracle':str(oracle),'ifm_sha256':sha,'installation_unchanged':True,'limitations':'Copied original nine-byte bit4 getter and21-byte setter without relocation. Supplied owned subobject at original owner+c118. Full propagation, live setter lifecycle and evaluation not executed.'}
Path('outputs/sfm64-document-propagation-flag-native-validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
