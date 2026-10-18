from pathlib import Path
import re, hashlib, json
from pe_image import Image

image = Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll')
instructions = []
for line in Path('work/studiorender-disassembly.txt').read_text(encoding='utf-8').splitlines():
    match = re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)', line)
    if match:
        instructions.append((int(match[1], 16), bytes.fromhex(match[2]), line.strip()))
slots = []
for slot in range(29):
    pointer = image.word(0x100710DC + slot * 4)
    body = []
    for address, prefix, line in instructions:
        if address < pointer: continue
        if address >= pointer + 512: break
        offset = image.offset(address)
        assert image.b[offset:offset+len(prefix)] == prefix
        body.append(line)
        if re.search(r'\bret\b', line): break
    slots.append(dict(slot=slot, function=hex(pointer), instructions=body))
report = dict(binary_sha256=hashlib.sha256(image.b).hexdigest(), vtable='0x100710dc', slots=slots,
              limitations='First return or 512-byte window only; displayed instruction prefixes checked, wrapped continuation bytes excluded. No runtime method oracle.')
Path('outputs/sfm64-studio-interface-slot-audit.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
for item in slots:
    print(item['slot'], item['function'])
    print('\n'.join(item['instructions'][:12]))
