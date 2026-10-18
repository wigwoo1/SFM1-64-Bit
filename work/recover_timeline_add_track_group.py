import hashlib
import json
import re
from pathlib import Path

source = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
start, end = 0x1017C560, 0x1017C7B0
spans = []
for line in Path('work/ifm-disassembly.txt').open():
    match = re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)', line)
    if not match:
        continuation = re.match(r'^\s{8,}((?:[0-9A-F]{2}(?:\s+|$))+)$', line.rstrip())
        if continuation and spans:
            spans[-1]['bytes'] += bytes.fromhex(continuation[1]).hex()
        continue
    va = int(match[1], 16)
    if va < start:
        continue
    if va >= end:
        break
    spans.append({'va': hex(va), 'bytes': bytes.fromhex(match[2]).hex(), 'instruction': match[3].strip()})

cursor = start
for row in spans:
    assert int(row['va'], 16) == cursor, (hex(cursor), row['va'])
    cursor += len(row['bytes']) // 2
assert cursor == end, (hex(cursor), hex(end))

binding_start, binding_end = 0x10176FC0, 0x1017700C
binding_spans = []
for line in Path('work/ifm-disassembly.txt').open():
    match = re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)', line)
    if not match:
        continuation = re.match(r'^\s{8,}((?:[0-9A-F]{2}(?:\s+|$))+)$', line.rstrip())
        if continuation and binding_spans:
            binding_spans[-1]['bytes'] += bytes.fromhex(continuation[1]).hex()
        continue
    va = int(match[1], 16)
    if va < binding_start:
        continue
    if va >= binding_end:
        break
    binding_spans.append({'va': hex(va), 'bytes': bytes.fromhex(match[2]).hex(), 'instruction': match[3].strip()})
cursor = binding_start
for row in binding_spans:
    assert int(row['va'], 16) == cursor, (hex(cursor), row['va'])
    cursor += len(row['bytes']) // 2
assert cursor == binding_end, (hex(cursor), hex(binding_end))

string_rows = {}
for line in Path('work/ifm-strings.txt').open():
    match = re.match(r'([0-9A-F]{8})\s+(.*)', line.rstrip())
    if match:
        string_rows[int(match[1], 16)] = match[2]
strings = {hex(va): string_rows[va] for va in (0x10AC4C70, 0x10AC5504, 0x10AC5518, 0x10AC552C)}

binary_hash = None
from pe_image import Image
image = Image(source)
for row in spans + binding_spans:
    offset = image.offset(int(row['va'], 16))
    raw = bytes.fromhex(row['bytes'])
    assert image.b[offset:offset + len(raw)] == raw, row['va']
binary_hash = hashlib.sha256(image.b).hexdigest()

receipt = {
    'binary': str(source),
    'binary_sha256': binary_hash,
    'action': 'Add Track Group',
    'action_member': 'CQTimelineEditor +0xF8',
    'action_callback': '0x1017C560',
    'callback_range': [hex(start), hex(end - 1)],
    'binding_range': [hex(binding_start), hex(binding_end - 1)],
    'verification_status': 'Directly byte-verified against installed DLL.',
    'disassembly_instructions': spans,
    'disassembly_instruction_count': len(spans),
    'disassembly_byte_count': sum(len(row['bytes']) // 2 for row in spans),
    'binding_instruction_count': len(binding_spans),
    'binding_byte_count': sum(len(row['bytes']) // 2 for row in binding_spans),
    'binding_instructions': binding_spans,
    'strings': strings,
    'observed_flow': [
        'The handler reads global timeline context member +0x344 and returns when the context or its resolved current object is absent.',
        'It constructs an input dialog titled Add Track Group and supplies the strings Track Group Name: and Track Group as dialog text/prompt inputs.',
        'It exits without creating anything when the dialog is rejected or required input is absent.',
        'On acceptance it resolves the entered value, invokes timeline/data-model methods, and dispatches update/notification operations.',
    ],
    'limitations': 'The handler range is byte-verified against the installed DLL. The precise Qt widget layout, value validation, transaction helper semantics, resulting DMElement class, and live menu/dialog behavior have not been exercised.',
}
Path('outputs/timeline-add-track-group-handler-recovery.json').write_text(json.dumps(receipt, indent=2))

menu_path = Path('outputs/timeline-trackgroup-menu-preparation-recovery.json')
menu = json.loads(menu_path.read_text())
menu['action_identities']['member_f8']['callback'] = '0x1017C560'
menu['action_identities']['member_f8']['evidence'] = ('The callable target and QAction registration range 0x10176FC0-0x1017700B plus complete handler range 0x1017C560-0x1017C7AF were directly byte-verified against installed ifm.dll SHA-256 ' + binary_hash + '.')
menu_path.write_text(json.dumps(menu, indent=2))
print(json.dumps({key: receipt[key] for key in ('action', 'action_callback', 'verification_status', 'binding_instruction_count', 'binding_byte_count', 'disassembly_instruction_count', 'disassembly_byte_count', 'binary_sha256', 'strings')}, indent=2))
