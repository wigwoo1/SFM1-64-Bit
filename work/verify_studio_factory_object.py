from pathlib import Path
import re,json,hashlib
from pe_image import Image

image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll')
raw=Path('work/studiorender-disassembly.txt').read_text(encoding='utf-8').splitlines()
decoded=[]
for line in raw:
    match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
    if match:
        decoded.append([int(match[1],16),bytes.fromhex(match[2]),line.strip()])
    elif decoded:
        continuation=re.fullmatch(r'\s+((?:[0-9A-F]{2} ?)+)\s*',line)
        if continuation: decoded[-1][1]+=bytes.fromhex(continuation[1])
ranges=[(0x1006F400,0x1006F416),(0x100586C0,0x100586F3),(0x1006F660,0x1006F66B)]
checked=[]
for begin,end in ranges:
    body=[]
    for address,data,line in decoded:
        if not begin<=address<end: continue
        offset=image.offset(address)
        assert image.b[offset:offset+len(data)]==data,(hex(address),data.hex())
        body.append(dict(address=hex(address),bytes=data.hex(),instruction=line))
    assert body
    checked.extend(body)
assert image.b[image.offset(0x100586ED):image.offset(0x100586ED)+6]==bytes.fromhex('C7 00 DC 10 07 10')
assert image.b[image.offset(0x1006F660):image.offset(0x1006F660)+11]==bytes.fromhex('C7 05 78 2E 87 10 DC 10 07 10 C3')
initializer_pointers=[]
for rva,size,start in image.sections:
    for offset in range(start,start+size-3,4):
        if image.u32(offset)==0x1006F400: initializer_pointers.append(hex(image.base+rva+offset-start))
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),factory_object='0x10872e78',initializer='0x1006f400',constructor='0x100586c0',vtable='0x100710dc',initializer_pointer_locations=initializer_pointers,verified_full_instructions=checked,
    conclusion='Initializer passes the factory-returned global object to a constructor which copies ecx to eax and stores recovered vtable at [eax]. The registered object and recovered table are statically connected.',
    limitations='No DLL startup execution, live object inspection, or runtime dispatch validation. Initializer pointer locations alone do not prove loader traversal.')
Path('outputs/sfm64-studio-factory-object-validation.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print('Verified',len(checked),'complete instructions; initializer pointers:',initializer_pointers)
