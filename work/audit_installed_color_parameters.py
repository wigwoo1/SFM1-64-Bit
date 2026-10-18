"""Verify installed color-space parameter registrations, not draw behavior."""
from pathlib import Path
import re,json,hashlib
from pe_image import Image
image=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\stdshader_dx9.dll')
strings={hex(address):image.string(address) for address in (0x1009ab04,0x10097aa8,0x1009ab14,0x1009ab44,0x1008b0cc)}
assert strings['0x1009ab04']=='$GAMMACOLORREAD'
assert strings['0x10097aa8']=='$LINEARWRITE'
assert strings['0x1008b0cc']=='0'
assert 'Disables SRGB conversion of color texture read.'==strings['0x1009ab14']
assert 'Disables SRGB conversion of shader results.'==strings['0x1009ab44']
rows=[];pending=None
with Path('work/stdshader_dx9-disassembly.txt').open() as source:
 for line in source:
  match=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
  if match:
   address=int(match[1],16);pending=None
   if 0x10083a20<=address<0x10083a3e or 0x10083a40<=address<0x10083a5e:
    pending={'address':address,'bytes':bytes.fromhex(match[2]).hex(),'instruction':match[3].strip()};rows.append(pending)
  elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):pending['bytes']+=bytes.fromhex(line).hex()
for start,end in ((0x10083a20,0x10083a3e),(0x10083a40,0x10083a5e)):
 cursor=start
 for row in rows:
  if not start<=row['address']<end:continue
  raw=bytes.fromhex(row['bytes']);assert row['address']==cursor
  offset=image.offset(cursor);assert image.b[offset:offset+len(raw)]==raw;cursor+=len(raw)
 assert cursor==end
report={'installed_stdshader_sha256':hashlib.sha256(image.b).hexdigest(),'strings':strings,'verified_registration_instructions':rows,'finding':'Installed module registers GAMMACOLORREAD and LINEARWRITE with default string0 and native descriptions saying they disable read/result sRGB conversion. Both shown registrations call0x1004cc20 into separate descriptor objects.', 'limits':'Registration metadata only; selected shader identity, parameter indices, runtime conditional read/write behavior and default state for other shaders remain unverified.'}
Path('outputs/installed-color-parameters-audit.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'PASS: two installed color-space registrations, {len(rows)} instructions/60bytes verified')
