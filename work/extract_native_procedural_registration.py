from pathlib import Path
lines=Path('work/current-datamodel-scale-disassembly.txt').read_text().splitlines()
matches=[n for n,line in enumerate(lines) if 'push        104EEAD8h' in line]
assert len(matches)==1
n=matches[0]
text='\n'.join(lines[n-30:n+45])+'\n'
Path('outputs/sfm64-native-procedural-setting-registration.txt').write_text(text)
print(text)
