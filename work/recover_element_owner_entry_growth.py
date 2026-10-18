import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
spans=[];active=None
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:
        c=re.fullmatch(r'\s{12}((?:[0-9A-F]{2} ?)+)\s*',line.rstrip())
        if c and active is not None:active['bytes']+=bytes.fromhex(c[1]).hex()
        continue
    va=int(m[1],16)
    if va>=0x10354072:break
    if va<0x10353fe0:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x10353fe0
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x10354072
Path('outputs/element-owner-entry-growth-recovery.json').write_text(json.dumps(dict(complete_bytes=146,instructions=spans,findings=[
 'Negative grow size skips allocation. Positive grow size rounds capacity to multiple; zero starts empty capacity4 and doubles.',
 'Stores capacity before allocator call, original byte count=capacity*8. Imported allocator10a7e484 virtual slot1 allocate/slot3 reallocate.',
 'Same normal growth policy as symbol pointer storage except initial capacity4 versus8 and element byte stride8 versus4.'
],limitations='Complete installed byte verification; overflow correction branches and allocator failure contract/original execution unverified. Portable normal-path storage uses sizeof(entry) with explicit initial capacity4.'),indent=2)+'\n')
print('Verified owner entry growth bytes:',146)
