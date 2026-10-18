from pathlib import Path
from pe_image import Image
import re,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf\bin\client.dll')
source=json.loads(Path('outputs/sfm64-client-weight-candidate.json').read_text(encoding='utf-8'))
assert source['binary_sha256']==hashlib.sha256(image.b).hexdigest()
selected=[]
for item in source['verified_complete_instructions']:
    address=int(item['address'],16)
    if not(0x1004899F<=address<=0x10048A25 or 0x10048E1A<=address<=0x10048E49):continue
    data=bytes.fromhex(item['bytes']);offset=image.offset(address);assert image.b[offset:offset+len(data)]==data
    selected.append(item)
offset=image.offset(0x10044BF0);assert image.b[offset:offset+7]==bytes.fromhex('FF 05 84 4E 6B 10 C3')
callers=[]
for line in Path('work/native-client-scale-disassembly.txt').read_text(encoding='utf-8').splitlines():
    if not re.search(r'\bcall\s+10044BF0\b',line):continue
    match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line);assert match
    data=bytes.fromhex(match[2]);offset=image.offset(int(match[1],16));assert image.b[offset:offset+len(data)]==data
    callers.append(line.strip())
report=dict(binary_sha256=source['binary_sha256'],generation_global='0x106b4e84',increment_function='0x10044bf0',checked_increment_bytes='ff05844e6b10c3',direct_increment_callers=callers,stored_generation='renderable_this+0xd5c',cached_current_pointer='renderable_this+0xd6c',cached_delayed_pointer='renderable_this+0xd80',verified_full_instructions=selected,limitations='Static cache/generation evidence. Counter scheduling, initialization, derived overrides, copy-helper exceptional paths and runtime SFM seeking remain unresolved.')
Path('outputs/sfm64-client-flex-generation.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print('Verified cache instructions:',len(selected),'direct generation increment callers:',len(callers))
print('\n'.join(callers))
