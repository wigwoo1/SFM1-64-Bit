"""Verify complete owner registration helper, without guessing manager type."""
import json, re
from pathlib import Path
from pe_image import Image
im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
start, end = 0x10356810, 0x103568c8
cursor = start
instructions = []
for line in Path('work/ifm-disassembly.txt').open():
    m = re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)', line)
    if not m:
        continue
    va = int(m[1], 16)
    if va >= end:
        break
    if va < start:
        continue
    raw = bytes.fromhex(m[2])
    assert va == cursor, (hex(va), hex(cursor))
    assert im.b[im.offset(va):im.offset(va)+len(raw)] == raw
    cursor += len(raw)
    instructions.append(dict(va=hex(va), bytes=raw.hex(), instruction=m[3].strip()))
assert cursor == end
report = dict(complete_routine_bytes=im.b[im.offset(start):im.offset(end)].hex(),
    verified_complete_instructions=instructions, findings=[
    'Second argument (element owner38) decoded as pool handle: low20 slot, next11 generation; high handle bit ignored except FFFFFFFF sentinel.',
    'Index checked unsigned against manager+c1ac; slot table manager+c1a0, eight-byte entries.',
    'Entry first word low31 must equal generation; high entry bit must be clear. Nonnull entry pointer+4 required.',
    '10356810 decrements and10356870 increments pointed object dword+10 with native32-bit wraparound; neither releases object nor clamps count.',
    'First argument (element1c) is unused throughout this complete88-byte helper.'
], limitations='Both complete88-byte helpers plus8 inter-routine padding bytes verified against installed PE. Manager class, owner object type, count meaning, pool construction/lifecycle, concurrency and original runtime execution remain unverified. This establishes a generational owner handle, not yet a proven file-ID binding.')
Path('outputs/element-owner-registration-recovery.json').write_text(json.dumps(report, indent=2)+'\n')
print('Verified complete owner registration bytes:', end-start, 'instructions:', len(instructions))
