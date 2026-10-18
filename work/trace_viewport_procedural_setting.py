from pathlib import Path
import json,re
lines=Path('work/viewport-procedural-disassembly.txt').read_text().splitlines()
matches=[n for n,line in enumerate(lines) if 'push        10237860h' in line];assert len(matches)==1
n=matches[0];registration='\n'.join(lines[n-12:n+25]);Path('outputs/sfm64-viewport-procedural-registration.txt').write_text(registration+'\n');print(registration)
records=[]
for n,line in enumerate(lines):
 if '00000104h' in line and ('byte ptr' in line or 'lea ' in line):
  match=re.match(r'\s*([0-9A-F]{8}):',line)
  if match:records.append(dict(address=match[1],instruction=line,context='\n'.join(lines[max(0,n-12):n+32])))
Path('outputs/sfm64-viewport-procedural-access-candidates.json').write_text(json.dumps(records,indent=2)+'\n');print('viewport +104h byte/lea candidates:',len(records))
for record in records:print(record['instruction'])
