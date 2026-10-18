import re,json,hashlib
from pathlib import Path
from pe_image import Image
records=[];names={}
for source,path,ranges in [('work/engine-disassembly.txt',r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll',[(0x10311730,0x10311745),(0x10311750,0x10311765)]),('work/ifm-disassembly.txt',r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll',[(0x103ba710,0x103ba716),(0x103ba7a1,0x103ba7c6),(0x10989dbf,0x10989dd7)])]:
 im=Image(path);rows=[];pending=None
 if 'engine-disassembly' in source:names['engine']=[im.string(0x1036683c),im.string(0x1036684c)]
 else:names['ifm']=im.string(0x10b42bec)
 for line in Path(source).open():
  m=re.match(r'^\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)(.*)',line)
  if m:
   va=int(m[1],16);pending=None
   if any(a<=va<z for a,z in ranges):pending={'address':hex(va),'bytes':m[2].strip(),'instruction':m[3].strip()};rows.append(pending)
  elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+\n?',line):pending['bytes']+=' '+line.strip()
 for row in rows:
  b=bytes.fromhex(row['bytes']);o=im.offset(int(row['address'],16));assert im.b[o:o+len(b)]==b
 records.append({'source':source,'sha256':hashlib.sha256(im.b).hexdigest(),'instructions':rows})
report={'interface_names':names,'checked_instructions':sum(len(r['instructions']) for r in records),'binding':'Engine registers VENGINETOOL003 and VENGINETOOLFRAMEWORK003 with same factory101E6470. IFM requests VENGINETOOL003 for10F4B728/accessor103BA710 and separately10FD4398.','limitations':'Factory request data flow only; runtime success, interface aliases and caller slot dispatch still need proof.','records':records}
Path('outputs/sfm64-document-tool-registration.json').write_text(json.dumps(report,indent=2));print('checked_instructions',report['checked_instructions'])
