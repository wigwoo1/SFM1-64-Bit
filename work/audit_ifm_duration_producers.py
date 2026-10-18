from pathlib import Path
from pe_image import Image
import hashlib, json, re, struct

image = Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
checked = 0
for line in Path('outputs/sfm64-ifm-duration-producers.txt').read_text().splitlines():
    match = re.match(r'  ([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)', line)
    if match:
        address = int(match[1], 16)
        expected = bytes.fromhex(match[2])
        offset = image.offset(address)
        assert image.b[offset:offset+len(expected)] == expected, hex(address)
        checked += 1
constants = {}
for address in (0x10a8dfd4, 0x10a8a87c, 0x10ab2848, 0x10a8a56c):
    word = image.word(address)
    constants[hex(address)] = dict(word=hex(word), value=struct.unpack('<f', struct.pack('<I',word))[0])
report = dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),
              checked_instruction_prefixes=checked, constants=constants,
              interface_global='0x10fd4398', setter_slot=50,
              duration_calls=['0x102aedc1','0x102aee72','0x102af2c8','0x102b2743',
                              '0x102b78ec','0x102b79fc','0x102b8050'],
              evidence_scope='Static duration producers; caller time semantics and live dispatch remain unverified')
Path('outputs/sfm64-ifm-duration-producer-audit.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
