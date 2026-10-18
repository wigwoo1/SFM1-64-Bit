"""Read-only installed UnlitTwoTexture host-state evidence; no DLL execution."""
from pathlib import Path
import hashlib
import json
import re
import struct
import tempfile
from pe_image import Image

ROOT = Path(__file__).resolve().parent.parent
INSTALLED = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin')


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def load(name, digest):
    path = INSTALLED / (name + '.dll')
    image = Image(path)
    require(hashlib.sha256(image.b).hexdigest() == digest, 'Installed binary changed: ' + name)
    return image, dict(path=str(path), sha256=digest)


def checked_ranges(image, name, ranges):
    """Check continuous disassembly bytes against current PE, including wrapped bytes."""
    rows = []
    pending = None
    with (ROOT / 'work' / (name + '-disassembly.txt')).open() as source:
        for line in source:
            match = re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)', line)
            if match:
                pending = None
                address = int(match[1], 16)
                if any(start <= address < end for _, start, end in ranges):
                    pending = dict(address=address, bytes=bytes.fromhex(match[2]).hex(),
                                   instruction=match[3].strip())
                    rows.append(pending)
            elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+', line):
                pending['bytes'] += bytes.fromhex(line).hex()
    result = []
    for label, start, end in ranges:
        selected = [row for row in rows if start <= row['address'] < end]
        cursor = start
        for row in selected:
            raw = bytes.fromhex(row['bytes'])
            require(row['address'] == cursor, 'Disassembly gap: ' + label)
            offset = image.offset(cursor)
            require(image.b[offset:offset+len(raw)] == raw, 'Stale disassembly: ' + label)
            cursor += len(raw)
        require(cursor == end, 'Incomplete range: ' + label)
        result.append(dict(name=label, start=hex(start), end_exclusive=hex(end),
                           byte_count=end-start, instructions=selected))
    return result


def exact(image, address, code):
    raw = bytes.fromhex(code)
    offset = image.offset(address)
    require(image.b[offset:offset+len(raw)] == raw, 'Contract bytes differ: ' + hex(address))
    return dict(address=hex(address), bytes=raw.hex())


host, host_record = load('stdshader_dx9', 'fb09e2ddc8a99aff4ef31e3fddfdb1ca84d89379336abdab4e76e94adfa7e6f8')
api, api_record = load('shaderapidx9', '07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa')
host_record['ranges'] = checked_ranges(host, 'stdshader_dx9', [
    ('family_name', 0x1004ce60, 0x1004ce66),
    ('shader_initialization', 0x1004ce80, 0x1004cf26),
    ('shader_draw', 0x1004cf30, 0x1004d5d3),
    ('blend_classification', 0x10002d20, 0x10002da6),
    ('alpha_modulation_flag', 0x10064dd0, 0x10064dd9),
    ('enable_alpha_blending', 0x10064e40, 0x10064e7a),
    ('disable_alpha_blending', 0x10064e80, 0x10064e90),
])
api_record['ranges'] = checked_ranges(api, 'shaderapidx9', [
    ('depth_write', 0x1002cd30, 0x1002cd44),
    ('alpha_write', 0x1002cde0, 0x1002cdf9),
    ('blend_enable', 0x1002ce00, 0x1002ce15),
    ('blend_factors', 0x1002cfb0, 0x1002cfd9),
    ('srgb_write', 0x1002d1a0, 0x1002d1d2),
    ('srgb_read', 0x1002d280, 0x1002d2d2),
])
require(host.string(host.word(0x100bbf84)) == 'UnlitTwoTexture_DX9', 'Family name differs')
for offset, target in [(0, 0x1004ce60), (0x34, 0x1004ce80), (0x38, 0x1004cf30)]:
    require(host.word(0x1009acac+offset) == target, 'Family vtable differs')
slots = {0x08: 0x1002cd30, 0x1c: 0x1002cde0, 0x20: 0x1002ce00,
         0x24: 0x1002cfb0, 0x4c: 0x1002d1a0, 0x50: 0x1002d280, 0x54: 0x1002d1e0}
for offset, target in slots.items():
    require(api.word(0x100febc4+offset) == target, 'Shader shadow vtable differs')
mapping = [1, 2, 9, 10, 5, 6, 7, 8, 11, 3, 4]
factor_checks = [exact(api, api.word(0x1002cf84+i*4), 'b8 '+struct.pack('<I', value).hex())
                 for i, value in enumerate(mapping)]
anchors = [exact(host, address, code) for address, code in [
    (0x1004d035, 'e8 e6 5c fb ff'),
    (0x1004d03a, '83 f8 03 74 19 83 f8 01 74 14'),
    (0x1004d04c, 'c1 ea 08 c6 45 b8 01 f6 c2 01 74 04 c6 45 b8 00'),
    (0x1004d066, '8b 50 54 6a 01 6a 00 8b cf ff d2'),
    (0x1004d073, '8b 50 50 6a 01 6a 00 8b cf ff d2'),
    (0x1004d080, '8b 50 54 6a 01 6a 01 8b cf ff d2'),
    (0x1004d08d, '8b 50 50 6a 01 6a 01 8b cf ff d2'),
    (0x1004d0a0, '8b 50 4c 6a 01 ff d2'),
    (0x1004d0f5, '6a 01 6a 01 eb 20'),
    (0x1004d113, '6a 01 eb 02 6a 05 6a 04 e8 20 7d 01 00'),
    (0x10064e4b, '8b 50 20 6a 01 ff d2'),
    (0x10064e5d, '8b 40 24 52 8b 55 08 52 ff d0'),
    (0x10064e6f, '8b 42 08 6a 00 ff d0'),
    (0x1004d2e0, '8b 4d b8 8b 07 8b 50 1c 51 8b cf ff d2'),
]]
receipt = dict(status='PASS_STATIC_CONTRACT', binaries=[host_record, api_record],
    family_vtable=hex(0x1009acac), shadow_vtable=hex(0x100febc4),
    shadow_slots={hex(k): hex(v) for k, v in slots.items()},
    native_blend_factor_mapping=mapping, factor_checks=factor_checks, anchors=anchors,
    findings=[
        'Uncloaked normal-pass snapshot requests both textures enabled, sRGB reads on samplers 0/1 and sRGB write. Native capability gates can clear sRGB state.',
        'No-alpha additive branch selects source ONE/destination ONE; alpha additive selects SRC_ALPHA/ONE; alpha nonadditive selects SRC_ALPHA/INV_SRC_ALPHA.',
        'Shared blend helper enables blending, forwards both factors and disables depth writes. It does not set depth test or depth comparison.',
        'Alpha write is driven separately by blend classification and alpha-test flag. Classification 2 (additive without classified alpha) keeps alpha writes enabled; do not universally disable alpha writes for additive.',
    ],
    limitations=[
        'Static byte-checked control flow only; no original shader DLL loaded or executed.',
        'No live pilot-light snapshot captured; effective modulation, flags, hardware capabilities and cloak state remain to be confirmed.',
        'No native D3D9/Vulkan pixel comparison, GPU integration, depth default recovery or full shader variant parity claimed.',
    ])
for image, record in [(host, host_record), (api, api_record)]:
    require(Path(record['path']).read_bytes() == image.b, 'Installed input changed during audit')
output = ROOT/'outputs/unlit-two-texture-host-state-20261014'
output.mkdir(exist_ok=True)
directory = Path(tempfile.mkdtemp(prefix='native-', dir=output))
(directory/'evidence.json').write_text(json.dumps(receipt, indent=2)+'\n')
count = sum(len(r['instructions']) for b in receipt['binaries'] for r in b['ranges'])
print(f'PASS: {count} byte-checked instructions; 7 shadow slots; 11 factor mappings')
print(directory/'evidence.json')
