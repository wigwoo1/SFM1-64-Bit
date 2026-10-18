import json,re
from pathlib import Path
from pe_image import Image
p=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
lines=Path('work/ifm-disassembly.txt').read_text().splitlines()
index={int(m.group(1),16):i for i,s in enumerate(lines) if (m:=re.match(r'\s*([0-9A-F]{8}):',s))}
def block(a,n=100):return '\n'.join(lines[index[a]:index[a]+n])
report=[]
for item in json.loads(Path('outputs/sfm-curveinfo-registrations.json').read_text()):
 getter=int(item['parent_getter'],16);text=block(getter,18)
 report.append(item['name']+' parent getter\n'+text)
Path('work/ifm-curveinfo-all-parent-getters.txt').write_text('\n\n'.join(report))
print('\n'.join(report[:2]))
print(block(0x10363530,45))
checks=[]
for item in json.loads(Path('outputs/sfm-curveinfo-registrations.json').read_text()):
 if item['name']=='DmeCurveInfo':continue
 factory=block(p.word(int(item['vtable'],16)),42)
 calls=re.findall(r'call\s+([0-9A-F]{8})',factory)
 ctor=int(calls[1],16)
 body=block(ctor,100)
 vtables=re.findall(r'mov\s+dword ptr \[esi\],([0-9A-F]{8})h',body)
 vt=int(vtables[0],16)
 checker=p.word(vt+0x10)
 check=block(checker,24)
 expected=item['symbol_global'][2:].upper()
 if expected not in check or ('10EB3340' not in check and item['name']!='DmeCurveInfo'):
  raise ValueError(item['name']+' unmatched chain')
 checks.append(item['name']+' constructor='+hex(ctor)+' instance_vtable='+hex(vt)+' checker='+hex(checker)+'\n'+check)
Path('work/ifm-curveinfo-instance-type-checkers.txt').write_text('\n\n'.join(checks))
print('verified_curveinfo_chains='+str(len(checks)))

