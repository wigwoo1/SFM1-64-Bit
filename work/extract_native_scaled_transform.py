from pathlib import Path
import re,json
lines=Path('work/datamodel-pyd-camera-disassembly.txt').read_text().splitlines();selected=[]
for line in lines:
 m=re.match(r'  ([0-9A-F]{8}):',line)
 if m and 0x100775C0<=int(m[1],16)<0x100776A0:selected.append(line)
Path('outputs/sfm64-native-scaled-transform-function.txt').write_text('\n'.join(selected)+'\n')
print('\n'.join(selected))
