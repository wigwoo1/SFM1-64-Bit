from pathlib import Path
import re
lines=Path('work/current-datamodel-scale-disassembly.txt').read_text().splitlines();contexts=[]
for n,line in enumerate(lines):
 match=re.match(r'\s*([0-9A-F]{8}):',line)
 if match and 0x10176000<=int(match[1],16)<0x10180000 and '00000108h' in line:
  contexts.append('\n'.join(lines[max(0,n-8):n+24]))
text='\n\n'.join(contexts)+'\n';Path('outputs/sfm64-native-procedural-setting-read-candidates.txt').write_text(text)
print('nearby member +108h read candidates:',len(contexts));print(text[:14000])
