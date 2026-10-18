from pathlib import Path
from pe_image import Image
import re, json, hashlib

im = Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
vtable = 0x10356a84
col = im.word(vtable - 4)
slots = {i: im.word(vtable + 4*i) for i in (6, 7, 8, 9, 12)}
ranges = [(p, p + 16) for p in set(slots.values())] + [(0x1017a360,0x1017a374),(0x1017c780,0x1017c8ac),(0x1017cf47,0x1017cfb7)]
lines = []
checked = 0
with Path('work/engine-disassembly.txt').open() as f:
    for line in f:
        m = re.match(r'  ([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)', line)
        if not m:
            continue
        va = int(m[1], 16)
        if va >= max(b for a, b in ranges):
            break
        if any(a <= va < b for a, b in ranges):
            prefix = bytes.fromhex(m[2])
            offset = im.offset(va)
            assert im.b[offset:offset+len(prefix)] == prefix
            checked += 1
            lines.append(line)
report = dict(engine_sha256=hashlib.sha256(im.b).hexdigest(), vtable=hex(vtable),
              type=im.string(im.word(col+12)+8), interface_offset=im.word(col+4),
              selected_slots={str(i):hex(p) for i,p in slots.items()},
              checked_instruction_prefixes=checked,
              diagnostic_strings={hex(p):im.string(p) for p in (0x103266cc,0x1032667c,0x103569e0,0x103569cc)},
              limitations='Static recovery; constructor tail extracted independently, not executed. Indirect imports and full playback lifecycle remain unresolved.')
Path('outputs/sfm64-alternate-clock-methods.txt').write_text(''.join(lines))
Path('outputs/sfm64-alternate-clock-methods-audit.json').write_text(json.dumps(report, indent=2))
print(json.dumps(report, indent=2))
print(''.join(lines))
