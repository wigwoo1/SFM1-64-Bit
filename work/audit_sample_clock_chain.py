from pathlib import Path
from pe_image import Image
from collections import deque
import re, json, hashlib
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
checked=0
for line in Path('outputs/sfm64-ifm-sample-clock-disassembly.txt').read_text().splitlines():
 m=re.match(r'  ([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
 if m:
  va=int(m[1],16);expected=bytes.fromhex(m[2]);o=im.offset(va)
  assert im.b[o:o+len(expected)]==expected,hex(va)
  checked+=1
strings={hex(va):im.string(va) for va in [0x10ae5880,0x10ae5874,0x10ae5864,0x10ae5858,0x10ae5848,0x10ae583c,0x10ae582c,0x10ae6488,0x10ae6464,0x10ae6444,0x10ae6414,0x10ae63e8]}
contexts=[];history=deque(maxlen=12);remaining=0
with Path('work/engine-disassembly.txt').open() as f:
 for line in f:
  if '103FF7B8h' in line:
   contexts.extend(history);remaining=14
  if remaining:contexts.append(line);remaining-=1
  history.append(line)
Path('outputs/sfm64-engine-current-clock-references.txt').write_text(''.join(contexts))
report=dict(checked_instruction_prefixes=checked,binary_sha256=hashlib.sha256(im.b).hexdigest(),strings=strings,
 evaluation_chain=['0x102ae740 -> 0x102ae0a0','0x102ae0a0 -> global10e9ccf8 slot9(+24) with selected operator list','0x102ae740 -> global10e9ccf8 slot11(+2c)'],
 direct_engine_tool_reference_in_extracted_methods=False,
 unresolved='Operator interface identity and current clock writers; no live timeline integration claimed')
Path('outputs/sfm64-sample-clock-chain-audit.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2));print(''.join(contexts)[-6000:])
