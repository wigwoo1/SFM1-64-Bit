from pathlib import Path
import json,re
lines=Path('work/current-datamodel-scale-disassembly.txt').read_text().splitlines();records=[]
for n,line in enumerate(lines):
 if '00000104h' in line and ('byte ptr' in line or 'lea ' in line):
  match=re.match(r'\s*([0-9A-F]{8}):',line)
  if match:records.append(dict(address=match[1],instruction=line,context='\n'.join(lines[max(0,n-5):n+15])))
Path('outputs/sfm64-native-procedural-bool-wide-candidates.json').write_text(json.dumps(records,indent=2)+'\n')
print('wide byte/lea access candidates:',len(records))
for record in records:print(record['instruction'])
