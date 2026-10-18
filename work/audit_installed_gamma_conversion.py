"""Read-only byte verification of the installed shader API gamma lookup."""
from pathlib import Path
import hashlib, json, re, struct
from pe_image import Image

image = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
digest = hashlib.sha256(image.b).hexdigest()
assert digest == '07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
rows = []
pending = None
with Path('work/shaderapidx9-disassembly.txt').open() as source:
    for line in source:
        match = re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)', line)
        if match:
            address = int(match[1], 16)
            pending = None
            if 0x100d4ef0 <= address < 0x100d4f37:
                pending = {'address': address, 'bytes': bytes.fromhex(match[2]).hex(), 'instruction': match[3].strip()}
                rows.append(pending)
        elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+', line):
            pending['bytes'] += bytes.fromhex(line).hex()
cursor = 0x100d4ef0
for row in rows:
    raw = bytes.fromhex(row['bytes'])
    assert row['address'] == cursor
    offset = image.offset(cursor)
    assert image.b[offset:offset+len(raw)] == raw
    cursor += len(raw)
assert cursor == 0x100d4f37
constants = {}
for address in (0x10152408, 0x100fc760):
    raw = image.b[image.offset(address):image.offset(address)+4]
    constants[hex(address)] = {'bytes': raw.hex(), 'float': struct.unpack('<f', raw)[0]}
assert constants['0x10152408']['float'] == struct.unpack('<f', struct.pack('<f', .95))[0]
assert constants['0x100fc760']['float'] == 255.0
report = {
    'installed_shaderapi_sha256': digest, 'verified_instructions': rows,
    'constants': constants, 'table_address': '0x101941d0',
    'finding': 'Installed routine returns zero below zero, one at the recovered float 0.95 threshold, otherwise multiplies by float 255 and indexes the fixed gamma table using x87 FISTP.',
    'rounding_limit': 'FISTP follows x87 control-word rounding. Existing SDK candidate uses SSE MXCSR rounding; equivalence requires matching rounding modes and is not proved by these bytes.',
    'scope_limit': 'Offline routine identification only. Material tint caller, runtime table initialization, execution parity and GPU integration remain unverified.'
}
destination = Path('outputs/installed-gamma-conversion-audit.json')
destination.write_text(json.dumps(report, indent=2)+'\n')
print(f'PASS: {len(rows)} instructions, {cursor-0x100d4ef0} bytes; constants verified; report={destination}')
