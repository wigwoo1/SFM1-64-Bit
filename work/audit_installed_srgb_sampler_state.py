"""Installed shader-shadow sampler setter, anchored by native diagnostic."""
from pathlib import Path
import re,json,hashlib
from pe_image import Image
image=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
digest=hashlib.sha256(image.b).hexdigest()
assert digest=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
assert image.word(0x100febc4+20*4)==0x1002d280
diagnostic=image.string(0x100fed64)
assert diagnostic=='Attempting set SRGBRead state on an invalid sampler (%d)!\n'
rows=[];pending=None
with Path('work/shaderapidx9-disassembly.txt').open() as source:
 for line in source:
  match=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
  if match:
   address=int(match[1],16);pending=None
   if 0x1002d280<=address<0x1002d2d2:
    pending={'address':address,'bytes':bytes.fromhex(match[2]).hex(),'instruction':match[3].strip()};rows.append(pending)
  elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):pending['bytes']+=bytes.fromhex(line).hex()
cursor=0x1002d280
for row in rows:
 raw=bytes.fromhex(row['bytes']);assert row['address']==cursor
 offset=image.offset(cursor);assert image.b[offset:offset+len(raw)]==raw
 cursor+=len(raw)
assert cursor==0x1002d2d2
assert any(row['bytes']=='6864ed0f10' for row in rows)
report={'installed_shaderapi_sha256':digest,'vtable':'0x100febc4','slot':20,'method':'0x1002d280','diagnostic_address':'0x100fed64','diagnostic':diagnostic,'verified_instructions':rows,
 'finding':'Native SRGBRead setter queries receiver+4 capability virtual slot26. Unsupported hardware clears owned bit2 at receiver+0x58+sampler without querying sampler count. Supported hardware queries count via virtual slot1, compares signed sampler<count, and updates bit2 from argument bit0, preserving other bits.',
 'limits':'Negative sampler is not rejected in recovered signed upper-bound comparison; replacement must reject negative indices. Static setter identity/effects only, not actual selected material values, capability implementation or rendered Source color-space parity. Same ShaderShadow010 interface name does not prove SDK vtable layout agreement.'}
Path('outputs/installed-srgb-sampler-state-audit.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'PASS: named native SRGBRead setter, slot20; {len(rows)} instructions/{cursor-0x1002d280} bytes verified')
