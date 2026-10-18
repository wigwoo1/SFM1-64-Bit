"""Verify LINEARWRITE branch, local flag uses and shadow-slot19 call."""
from pathlib import Path
import re,json,hashlib
from pe_image import Image
shader=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\stdshader_dx9.dll')
api=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
assert api.word(0x100febc4+19*4)==0x1002d1a0
def recover(filename,image,ranges):
 rows=[];pending=None
 with Path(filename).open() as source:
  for line in source:
   match=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
   if match:
    address=int(match[1],16);pending=None
    if any(start<=address<end for start,end in ranges):
     pending={'address':address,'bytes':bytes.fromhex(match[2]).hex(),'instruction':match[3].strip()};rows.append(pending)
   elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):pending['bytes']+=bytes.fromhex(line).hex()
 for start,end in ranges:
  cursor=start
  for row in rows:
   if not start<=row['address']<end:continue
   raw=bytes.fromhex(row['bytes']);assert row['address']==cursor
   offset=image.offset(cursor);assert image.b[offset:offset+len(raw)]==raw;cursor+=len(raw)
  assert cursor==end
 return rows
branch=recover('work/stdshader_dx9-disassembly.txt',shader,[(0x100555cc,0x100555ee),(0x10055cd2,0x10055ce2)])
setter=recover('work/shaderapidx9-disassembly.txt',api,[(0x1002d1a0,0x1002d1d2)])
flag_references=[]
with Path('work/stdshader_dx9-disassembly.txt').open() as source:
 for line in source:
  match=re.match(r'\s*([0-9A-F]{8}):',line)
  if match and 0x100555cc<=int(match[1],16)<0x10055ce2 and 'FFFFFB6Ch]' in line:flag_references.append(line.strip())
assert len(flag_references)==5
assert sum("mov         byte ptr" in reference for reference in flag_references)==2
report={'stdshader_sha256':hashlib.sha256(shader.b).hexdigest(),'shaderapi_sha256':hashlib.sha256(api.b).hexdigest(),'branch_and_call':branch,'setter':setter,'local_flag_references':flag_references,
 'finding':'Helper field0x138 selects parameter index. Branch initializes local flag1; index!=-1 and parameter+8==1 sets0. The same local flag is later passed to shadow virtual slot19, whose installed target gates receiver+0x6c bit0x80 by hardware capability slot26.',
 'interpretation':'Together with verified LINEARWRITE registration description and field transfer, this supports a native output sRGB enable policy: disabled for value1, enabled otherwise, then hardware-gated.',
 'limits':'Static chain; selected shader/material instance and actual draw/output encoding not traced. Local-reference inventory excludes alias writes; whole-program alias proof and runtime execution of this caller remain unverified.'}
Path('outputs/installed-linear-write-branch-audit.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'PASS: {len(branch)} branch/call and {len(setter)} setter instructions;100bytes verified')
