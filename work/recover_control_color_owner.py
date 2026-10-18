import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
assert im.string(0x10aa6704)=='controls'
spans=[]
for line in Path('work/ifm-disassembly.txt').open():
 m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
 if not m:continue
 va=int(m[1],16)
 if va>=0x105cc098:break
 if va<0x105cc060:continue
 raw=bytes.fromhex(m[2]);at=im.offset(va)
 assert im.b[at:at+len(raw)]==raw
 spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
receipt=dict(verified_prefixes=spans,name='controls',findings=[
 '105cc060 interns controls via manager virtualc8 on every invocation, gets returned symbol and calls1008cf90(element,symbol,1,0).',
 'Therefore control color owner is first incoming CDmeControlGroup through controls attribute with same owner38 identity; restriction0 means attribute800/1000 flags not filtered.',
 'Uses same iterator/filter as inherited visibility, changing requested attribute from children to controls. Does not infer owning group from displayed node parent.'
 ],limitations='First-line prefixes and attribute string verified. Symbol interning/manager iterator binding/original runtime parity pending.')
Path('outputs/control-color-owner-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified color owner prefixes:',len(spans))
