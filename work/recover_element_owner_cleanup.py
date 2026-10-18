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
    if va>=0x1035b781:break
    if va<0x1035b660:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x1035b660
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x1035b781
Path('outputs/element-owner-cleanup-recovery.json').write_text(json.dumps(dict(complete_bytes=289,instructions=spans,findings=[
 'Global10e9a51c required nonnull: virtualslot61 returns byte saved, slot60 called with0 before scan and saved byte restored through slot60 afterward.',
 'Captures signed elementpool countc0d0 after service disable, then ascending slots; fetches current elementpool tablec0c4 each iteration, skips nullpointerentries.',
 'Constructs22-bit slot/9-bit generation elementhandle, rejects unrepresentable index/sentinel, resolves via CDataModel slot18, tests exact element38==supplied ownerhandle.',
 'Matching element invokes1035b220(handle, mode): secondargumentbyte nonzero -> mode0, zero -> mode2.',
 'After scan resolves current ownerpool with full sentinel/generation/inactive/nonnull guards and writes markedbytec=0; service restore occurs even invalid owner.',
 'Element handle highbit carried from stacklocal ebp-4, which is not initialized before first nonnullentry; no stable semantic flag inferred. Service restore pushes32-bit local with only lowbyte initialized; slot60 argument ABI still pending.'
],limitations='Complete installed byte verification; service ABI/state meaning, element deletion helper, opaque highbits, original execution and production session integration unresolved. Do not label service as undo or notification without evidence.'),indent=2)+'\n')
print('Verified owner cleanup bytes:',289)
