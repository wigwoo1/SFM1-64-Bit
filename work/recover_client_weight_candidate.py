from pathlib import Path
from pe_image import Image
import re,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf\bin\client.dll')
assert image.word(0x104C84EC+14*4)==0x10048920
assert image.word(0x104C84EC+39*4)==0x10045660
decoded=[]
for line in Path('work/native-client-scale-disassembly.txt').read_text(encoding='utf-8').splitlines():
    match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
    if match:decoded.append([int(match[1],16),bytes.fromhex(match[2]),line.strip()])
    elif decoded:
        tail=re.fullmatch(r'\s+((?:[0-9A-F]{2} ?)+)\s*',line)
        if tail:decoded[-1][1]+=bytes.fromhex(tail[1])
selected=[]
for address,data,line in decoded:
    if not (0x10048920<=address<0x10048E70 or 0x10045660<=address<0x1004567D or 0x1045CE30<=address<0x1045CE90 or 0x1045D509<=address<0x1045D660 or 0x1045D660<=address<0x1045D700):continue
    offset=image.offset(address);assert image.b[offset:offset+len(data)]==data
    selected.append(dict(address=hex(address),bytes=data.hex(),instruction=line))
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),class_name='C_BaseFlex',subobject_offset=4,vtable='0x104c84ec',weight_candidate='0x10048920',delayed_predicate_candidate='0x10045660',verified_complete_instructions=selected,limitations='RTTI and matching slot evidence; actual engine object dispatch, weight algorithm and derived class overrides remain unverified. Extraction end is a bounded window, not a proven complete function extent.')
Path('outputs/sfm64-client-weight-candidate.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print('Verified',len(selected),'complete client instructions')
