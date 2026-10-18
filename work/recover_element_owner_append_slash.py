import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
start,end=0x109760d0,0x1097611c
cursor=start;spans=[]
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:continue
    va=int(m[1],16)
    if va>=end:break
    if va<start:continue
    raw=bytes.fromhex(m[2]);assert va==cursor
    assert im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw);spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
assert cursor==end
report=dict(verified_complete_instructions=spans,complete_routine_bytes=end-start,
 findings=[
 '109760d0 V_AppendSlash leaves empty strings and either trailing separator unchanged.',
 'For nonempty path lacking separator, signed length+1 must be less than capacity to fit backslash plus terminator.',
 'Failure invokes imported diagnostic10a7e474 then writes backslash/NUL if diagnostic returns; original nonreturn contract must be verified before production use.',
 'Does not normalize earlier slash characters or append separator to empty path.'
 ],limitations='Complete76-byte routine verified against installed binary. Imported diagnostic implementation/runtime contract and pathological signed length overflow unverified.')
Path('outputs/element-owner-append-slash-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified complete append slash bytes:',end-start)
