"""Verify completeobject100144a0 auxiliary pixel-constant helper."""
from pathlib import Path
import hashlib,json,re
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
start,end=0x100144a0,0x100146a6
covered={};address=None
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)',line)
 if m:address=int(m[1],16);raw=bytes.fromhex(m[2])
 elif address is not None and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):raw=bytes.fromhex(line)
 else:continue
 for b in raw:
  if start<=address<end:covered[address]=b
  address+=1
assert len(covered)==end-start
raw=bytes(covered[a] for a in range(start,end))
assert raw==im.b[im.offset(start):im.offset(start)+len(raw)]
report={'body':[hex(start),hex(end)],'verified_bytes':len(raw),'installed_sha256':hashlib.sha256(im.b).hexdigest(),
 'entry':'Publish first argument register index at owner+3610 before calling secondary(owner+2d0) slot2. Active iff returned mode nonzero and second argument nonzero.',
 'active_values':'lo=owner+2f4c, hi=owner+2f50. UCOMISS/LAHF/test44/JNP skips division for ordered equality; ordered inequality and unordered execute inverse=1/(hi-lo). Float4={1-hi*inverse,owner+18a0,clamped owner+2f54,inverse}. COMISS upper/lower comparisons retain NaN on masked exception path; exception behavior unverified.',
 'inactive_values':['zero',hex(im.word(0x100fc7dc)),'zero','zero'],
 'comparison':'Four raw DWORDs compared to cache pointer owner+2864 at first argument*16. Unchanged returns; index retained across submission callbacks.',
 'submission':'Changed count1: opcode7,index,1,fourwords via owner+648 command state, original10002da0 if remaining<7; otherwise COM109 SetPixelShaderConstantF.',
 'publication':'Reload current pointer owner+19ec after submission and copy16bytes at retained index; then reload comparison pointer owner+2864 and copy same local16bytes.',
 'limitations':'Static verification only. Register/index valid domain, cache capacity/lifetime, FP exceptions/rounding and live callbacks need native execution. No complete shader transition or GPU parity claimed.'}
Path('outputs/sfm64-shader-aux-constant-recovery.json').write_text(json.dumps(report,indent=2))
print('verified_shader_aux_constant_bytes='+str(len(raw)))
