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
    if va>=0x102a245b:break
    if va<0x102a2390:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x102a2390
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x102a245b
assert im.string(0x10ae5a40)=='wait; map %s\n'
assert im.string(0x10ae698c)=="Unable to load map '%s'\n"
Path('outputs/editor-map-command-recovery.json').write_text(json.dumps(dict(complete_bytes=203,instructions=spans,findings=[
 '102a2390 refuses when this16 nonzero. Null supplied map name falls back102a2210; copies name to256-byte local via10975a20.',
 'Engine global10fd4398 slot27 checks supplied/copy map. False returnsfalse with no pending flag or command.',
 'On true sets this16=1, formats256-byte command using installed wait; map %s newline format at10ae5a40, engine slot23 dispatches, slot24 invoked, editor10275f50 invoked, returns AL1.',
 'Caller102b6180 invokes this helper at102b632c; false emits installed Unable to load map diagnostic10ae698c. Establishes map loading branch in verified suppression scope, not full editor class identity.'
],limitations='Complete bytes/formats verified; engine interface identity, bounded string helper exact behavior, async completion/pending reset and native editor integration unresolved.'),indent=2)+'\n')
print('Verified editor map command bytes:',203)
