from pathlib import Path
import hashlib, json, struct
from pe_image import Image

im = Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
references = {}
for target in (0x1017e040, 0x1017e8b8, 0x1017f188, 0x1017f290,
               0x10180aa8, 0x10180ab8, 0x10180abc, 0x10180ac0, 0x10180ac4):
    needle = struct.pack('<I', target)
    hits = []
    for rva, size, raw in im.sections:
        start = raw
        while True:
            at = im.b.find(needle, start, raw + size)
            if at < 0:
                break
            hits.append(hex(im.base + rva + at - raw))
            start = at + 1
    references[hex(target)] = hits

strings = {hex(a): im.string(a) for a in (0x100fa430, 0x100fa444, 0x100fa3c8)}
assert strings['0x100fa430'] == 'mem_dumpvballocs'
getter = im.b[im.offset(0x1000df50):im.offset(0x1000df50)+6]
assert getter == bytes.fromhex('b8 a8 0a 18 10 c3')
report = {
    'sha256': hashlib.sha256(im.b).hexdigest(),
    'exact_absolute_address_occurrences': references,
    'nearby_startup_strings': strings,
    'command_state_getter': {'address': '0x1000df50', 'bytes': getter.hex(),
                             'returns': '0x10180aa8'},
    'command_state_fields': {'queue_enabled': '0x10180ab8',
                            'block': '0x10180abc', 'cursor': '0x10180ac0',
                            'available_words': '0x10180ac4'},
    'finding': '100f9190 registers mem_dumpvballocs through100d6250, with diagnostic callback10002cd0. It is not evidence of command block pool initialization.',
    'limitations': 'Exact address occurrence scan includes instruction operands and data without semantic classification. It cannot exclude relative addressing, indirect initialization, external users, or runtime-populated pointers. Command pool ownership and consumer remain unresolved.'
}
Path('outputs/sfm64-device-command-pool-reference-audit.json').write_text(json.dumps(report, indent=2))
print(json.dumps(references))
