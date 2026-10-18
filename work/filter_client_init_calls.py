from pathlib import Path
lines=Path('outputs/sfm64-engine-client-interface-call-contexts.txt').read_text().splitlines()
for i,line in enumerate(lines):
 if 'mov         edx,dword ptr [eax]' in line or 'mov         eax,dword ptr [edx]' in line or 'call        dword ptr [eax]' in line:print('\n'.join(lines[max(0,i-5):i+12]))
