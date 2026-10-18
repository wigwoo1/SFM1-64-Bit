import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions=[(0x105cb270,0x105cb398),(0x105cb400,0x105cb436)]
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
bindings=[]
for a,name,wrapper,attr,setter in [(0x10b95e88,'selectable',0xac,0xb0,0x105cb400),(0x10b95e7c,'snappable',0xb4,0xb8,0x105cb420)]:
    assert im.string(a)==name
    bindings.append(dict(name=name,string_va=hex(a),wrapper=hex(wrapper),attribute_pointer=hex(attr),setter=hex(setter),type_code=4))
for a,name,wrapper,attr,dtype in [(0x10a8a7d4,'children',0x5c,0x74,15),(0x10aa6704,'controls',0x78,0x90,15),(0x10b95ea4,'groupColor',0x94,0x98,8),(0x10b95e94,'controlColor',0x9c,0xa0,8),(0x10aa7d40,'visible',0xa4,0xa8,4)]:
    assert im.string(a)==name
    bindings.append(dict(name=name,string_va=hex(a),wrapper=hex(wrapper),attribute_pointer=hex(attr),type_code=dtype))
receipt=dict(verified_prefixes=spans,bindings=bindings,findings=[
    'Initializer105cb270 binds children/controls arrays type15; children has CDmeControlGroup type metadata, controls global10e9a684 metadata whose identity remains pending.',
    'groupColor/controlColor type8 initialized to packedffc8c8c8 via1036cd90.',
    'Visible/selectable/snappable type4 initialize to true: bl explicitly set1 at105cb326 and preserved across callee calls by x86 ABI.',
    '105cb400 loads selectable attributeB0 and passes address of argument to10366260.105cb420 independently uses snappableB8.',
    'UI setData column3 previously recovered calls105cb400. This setter does not visibly recurse children; UI ancestor/descendant handling and separate helper paths must remain at caller layer.'
],limitations='First-line prefixes and strings verified against original PE. Color channel interpretation, controls metadata identity, bool assignment notifications and full native UI runtime parity pending.')
Path('outputs/control-group-selectable-binding-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
