from pathlib import Path
from pe_image import Image
import re,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
assert image.string(0x103614B4)=='VStudioRender026'
body=[]
for line in Path('work/engine-disassembly.txt').read_text(encoding='utf-8').splitlines():
    match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
    if not match or not 0x1024E3F0<=int(match[1],16)<=0x1024E410: continue
    data=bytes.fromhex(match[2]);offset=image.offset(int(match[1],16))
    assert image.b[offset:offset+len(data)]==data
    body.append(line.strip())
assert len(body)==15
Path('outputs/sfm64-engine-studio-interface-lookup.json').write_text(json.dumps(dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),interface='VStudioRender026',interface_address='0x103614b4',lookup='0x1024e3f0',result_global='0x106a7510',verified_instructions=body,limitations='Static factory-loop evidence; successful runtime lookup and consumers of the global remain unverified.'),indent=2)+'\n',encoding='utf-8')
print('Verified factory lookup and result store:',len(body),'instructions')
