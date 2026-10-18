from pathlib import Path
import json
path=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\sdktools\python\2.7\win32\Lib\site-packages\vs\movieobjects.py')
lines=path.read_text(errors='replace').splitlines();records=[];current=''
for index,line in enumerate(lines):
 if line.startswith('class '):current=line.split('(')[0].split()[1]
 if 'scale' in line.lower() and ('CDmeTransform' in current or 'CDmeGameModel' in current):
  records.append(dict(line=index+1,owner=current,text=line.strip()))
result=dict(source=str(path),scale_related_transform_or_model_bindings=records,conclusion='Published SDK transform header exposes position and orientation only. Installed Python bindings must be inspected separately from binary implementation; absence of a scale method does not prove the dynamic scale attribute is ignored.',sdk_header='https://raw.githubusercontent.com/ValveSoftware/source-sdk-2013/master/src/public/movieobjects/dmetransform.h',native_scale_composition='unverified')
Path('outputs/sfm64-native-bone-scale-binding-audit.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
