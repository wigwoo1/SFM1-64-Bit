"""Installed parameter-index transfer and conditional sampler0 SRGBRead call."""
from pathlib import Path
import re,json
import audit_installed_color_parameters as registration
image=registration.image
ranges=[(0x1004ca0a,0x1004ca22),(0x10055b00,0x10055b31)]
rows=[];pending=None
with Path('work/stdshader_dx9-disassembly.txt').open() as source:
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
report={
 'installed_stdshader_sha256':registration.report['installed_stdshader_sha256'],
 'verified_instructions':rows,
 'parameter_transfer':{'gammacolorread_descriptor_index':'0x100c914c','helper_field':'0x13c','linearwrite_descriptor_index':'0x100c91f4','helper_field_linearwrite':'0x138'},
 'gamma_read_branch':'When the surrounding texture-present gate permits entry, helper index -1 or parameter+8 value other than1 selects enabled1; value1 selects enabled0. Pushes sampler0 and calls receiver virtual slot20 (offset0x50), independently identified in shaderapi as SRGBRead.',
 'limits':'Static field transfer and branch, not full interprocedural proof of selected material/shader identity. Parameter+8 integer representation observed; schema and unusual types require additional verification. LINEARWRITE runtime call is not proved here.'
}
Path('outputs/installed-gamma-read-branch-audit.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'PASS: {len(rows)} transfer/branch instructions, 73 bytes verified; sampler0 calls slot20')
