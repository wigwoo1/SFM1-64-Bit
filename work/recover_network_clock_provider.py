from pathlib import Path
from pe_image import Image
import re,json,hashlib
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\networksystem.dll')
table=0x10031504;col=im.word(table-4);method=im.word(table+43*4)
ranges=[(0x10009070,0x10009076),(0x1002df20,0x1002df55),(0x1000f580,0x1000f600),(method,method+32)]
decoded=[]
for line in Path('work/networksystem-disassembly.txt').read_text().splitlines():
 m=re.match(r'  ([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
 if m:decoded.append([int(m[1],16),bytes.fromhex(m[2]),line])
 elif decoded:
  c=re.fullmatch(r'\s+((?:[0-9A-F]{2} ?)+)\s*',line)
  if c:decoded[-1][1]+=bytes.fromhex(c[1])
selected=[]
for va,b,line in decoded:
 if any(a<=va<z for a,z in ranges):
  o=im.offset(va);assert im.b[o:o+len(b)]==b;selected.append(dict(address=hex(va),bytes=b.hex(),instruction=line.strip()))
report=dict(binary_sha256=hashlib.sha256(im.b).hexdigest(),factory='0x10009070',object='0x1004b7f8',constructor='0x1000f580',vtable=hex(table),type=im.string(im.word(col+12)+8),object_offset=im.word(col+4),slot43=hex(method),verified_full_instructions=selected,limitations='Static factory/constructor linkage; no initialization or live dispatch execution.')
Path('outputs/sfm64-network-clock-provider.json').write_text(json.dumps(report,indent=2))
print(json.dumps({k:v for k,v in report.items() if k!='verified_full_instructions'},indent=2))
print('\n'.join(x['instruction'] for x in selected if method<=int(x['address'],16)<method+32))
