import json,re,struct
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions=[(0x1008d2d0,0x1008d34d),(0x104d4a10,0x104d4a25),(0x104d86f0,0x104d86ff),(0x105cc9a0,0x105cc9af)]
spans=[]
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:continue
    va=int(m[1],16)
    if va>=regions[-1][1]:break
    if not any(a<=va<b for a,b in regions):continue
    raw=bytes.fromhex(m[2]);at=im.offset(va)
    assert im.b[at:at+len(raw)]==raw
    spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
factories=[]
for setter,glob in [(0x104d86f0,0x10eb1ac4),(0x105cc9a0,0x10eb61c4)]:
    needle=struct.pack('<I',setter);matches=[]
    for rva,size,raw in im.sections:
        at=im.b.find(needle,raw,raw+size)
        while at!=-1:
            va=im.base+rva+at-raw;vt=va-0x14
            try:
                col=im.word(vt-4);td=im.word(col+12);name=im.string(td+8)
                if name.startswith('.?AV?$CDmElementFactory@'):matches.append(dict(vtable=hex(vt),rtti=name,locator=hex(col),descriptor=hex(td)))
            except (ValueError,UnicodeError):pass
            at=im.b.find(needle,at+1,raw+size)
    assert len(matches)==1
    factories.append(dict(setter=hex(setter),type_global=hex(glob),**matches[0]))
receipt=dict(verified_prefixes=spans,factories=factories,findings=[
    '1008d2d0 accepts kind3 directly only after nodehandle0c resolves and type test against10eb61c4 passes.',
    'Kind2 requires resolved nodehandle0c and type test10eb1ac4, then resolves its handlecc via104d4a10. No additional returned-object type guard appears in104d4a10.',
    'Other kinds, missing objects and failed type tests return null. No ancestor walk or fallback to shared control record in this body.',
    'This lookup determines column3 interaction flags and edit targets; native graph adapter must retain kind-specific paths.'
],limitations='First-line prefixes and factory RTTI verified against installed PE. Handlecc binding name and full runtime graph/view parity remain unresolved.')
Path('outputs/control-model-group-lookup-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans));print(factories)
