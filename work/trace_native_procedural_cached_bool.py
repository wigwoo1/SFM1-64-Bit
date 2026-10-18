from pathlib import Path
import re,json
lines=Path('work/current-datamodel-scale-disassembly.txt').read_text().splitlines();records=[]
for n,line in enumerate(lines):
 m=re.match(r'\s*([0-9A-F]{8}):',line)
 if m and 0x10177000<=int(m[1],16)<0x1017B000 and ('00000104h' in line or '00000108h' in line):
  records.append(dict(address=m[1],instruction=line,context='\n'.join(lines[max(0,n-12):n+35])))
Path('outputs/sfm64-native-procedural-cached-bool-candidates.json').write_text(json.dumps(records,indent=2)+'\n')
print('candidate accesses:',len(records))
for r in records:print(r['context']+'\n')
