import sys,re,json,struct
sys.path.insert(0,'work');from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');anchors={}
for m in re.finditer(rb'[ -~]*[Hh]ead(?:Locked|IsLocked|isLocked)[ -~]*\x00',im.b):
 off=m.start();sec=next(((rv,sz,ro) for rv,sz,ro in im.sections if ro<=off<ro+sz),None)
 if sec:
  rv,sz,ro=sec;anchors[im.base+rv+off-ro]=m.group()[:-1].decode('ascii')
refs=[]
from pathlib import Path
for line in Path('work/ifm-disassembly.txt').open():
 m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
 if not m:continue
 raw=bytes.fromhex(m[2]);va=int(m[1],16)
 for i in range(max(0,len(raw)-3)):
  target=struct.unpack_from('<I',raw,i)[0]
  if target in anchors:
   at=im.offset(va);assert im.b[at:at+len(raw)]==raw
   refs.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip(),text=anchors[target]))
report=dict(anchors={hex(k):v for k,v in anchors.items()},references=refs,limitations='Immediate string references only; connection pairings and read semantics need body recovery.')
Path('outputs/head-lock-signal-read-references.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
