from pathlib import Path
from pe_image import Image
import re, json, hashlib, struct

image = Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll')
lines = Path('work/studiorender-disassembly.txt').read_text(encoding='utf-8').splitlines()
found = []
for match in re.finditer(rb'VStudioRender[^\x00]*\x00', image.b):
    raw = match.start()
    address = next(image.base+rva+raw-start for rva,size,start in image.sections if start <= raw < start+size)
    references = []
    for index,line in enumerate(lines):
        if f'{address:08X}h' not in line: continue
        instructions = []
        for nearby in lines[max(0,index-10):index+11]:
            parsed = re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',nearby)
            if not parsed: continue
            prefix = bytes.fromhex(parsed[2]); offset=image.offset(int(parsed[1],16))
            assert image.b[offset:offset+len(prefix)] == prefix
            instructions.append(nearby.strip())
        references.append(instructions)
    pointers=[]
    for rva,size,start in image.sections:
        for offset in range(start,start+size-3,4):
            if image.u32(offset)==address:
                pointers.append(hex(image.base+rva+offset-start))
    found.append(dict(name=match[0][:-1].decode('ascii'),address=hex(address),aligned_pointers=pointers,reference_windows=references))
checked_routines={}
for begin,end in [(0x10052C30,0x10052C36),(0x1005ADC0,0x1005ADE2)]:
    body=[]
    for line in lines:
        parsed=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
        if not parsed or not begin<=int(parsed[1],16)<end: continue
        prefix=bytes.fromhex(parsed[2]); offset=image.offset(int(parsed[1],16))
        assert image.b[offset:offset+len(prefix)]==prefix
        body.append(line.strip())
    assert body
    checked_routines[hex(begin)]=body
assert image.b[image.offset(0x10052C30):image.offset(0x10052C30)+6]==bytes.fromhex('B8 78 2E 87 10 C3')
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),interfaces=found,
            checked_routines=checked_routines,factory='0x10052c30',factory_object='0x10872e78',
            registration_node='0x10873268',registration_list_head='0x1088a984',
            limitations='Offline string/reference and linked registration evidence; displayed prefixes checked; no live factory invocation or initialized object vptr inspection.')
Path('outputs/sfm64-studio-interface-registration.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps(report,indent=2))
