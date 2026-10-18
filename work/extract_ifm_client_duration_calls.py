from pathlib import Path
blocks=[];active=[];remaining=0
with Path('work/ifm-disassembly.txt').open() as f:
 for line in f:
  if '10F4B728h' in line or '10FD4398h' in line:
   if active:blocks.append(active)
   active=[];remaining=20
  if remaining:active.append(line);remaining-=1
  if not remaining and active:blocks.append(active);active=[]
selected=[b for b in blocks if any('+000000C8h]' in x or '+0C8h]' in x for x in b)]
Path('outputs/sfm64-ifm-client-frame-duration-calls.txt').write_text(''.join(''.join(b) for b in selected));print('contexts',len(blocks),'duration setter candidates',len(selected));print(''.join(''.join(b) for b in selected)[:12000])
