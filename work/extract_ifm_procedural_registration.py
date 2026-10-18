from pathlib import Path
lines=Path('work/ifm-disassembly.txt').read_text().splitlines();matches=[n for n,line in enumerate(lines) if 'push        10B94644h' in line];assert len(matches)==1
n=matches[0];text='\n'.join(lines[n-14:n+28])+'\n';Path('outputs/sfm64-ifm-procedural-registration.txt').write_text(text);print(text)
