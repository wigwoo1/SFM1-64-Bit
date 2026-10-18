from pathlib import Path
lines=Path('work/ifm-disassembly.txt').read_text().splitlines();out=[]
for i,s in enumerate(lines):
 if '10F60300h]' in s and any(m in s for m in ['mov         dword ptr ds:','movss       dword ptr ds:','fstp        dword ptr ds:']):out.extend(lines[max(0,i-6):i+7])
Path('work/ifm-float-compressed-fallback-writes.txt').write_text('\n'.join(out)+'\n');print('\n'.join(out[:65]));print('write context lines',len(out))
