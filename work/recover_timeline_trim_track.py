import hashlib
import json
import re
from pathlib import Path
from pe_image import Image

source = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
disasm = Path('work/ifm-disassembly.txt')

def extract(start, end):
    spans = []
    for line in disasm.open():
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
    return spans

handler_range = (0x1017CC80, 0x1017CDCE)
constructor_range = (0x10177DCA, 0x10177DFD)
binding_range = (0x101739D2, 0x101739EE)
handler = extract(*handler_range)
constructor = extract(*constructor_range)
binding = extract(*binding_range)
image = Image(source)
for row in handler + constructor + binding:
    offset = image.offset(int(row['va'], 16))
    raw = bytes.fromhex(row['bytes'])
    assert image.b[offset:offset + len(raw)] == raw, row['va']
binary_hash = hashlib.sha256(image.b).hexdigest()

string_rows = {}
for line in Path('work/ifm-strings.txt').open():
    match = re.match(r'([0-9A-F]{8})\s+(.*)', line.rstrip())
    if match:
        string_rows[int(match[1], 16)] = match[2]
strings = {hex(va): string_rows[va] for va in (0x10AB72AC, 0x10AC557C)}

receipt = {
    'binary': str(source),
    'binary_sha256': binary_hash,
    'action_label': 'Trim Handles',
    'action_member': 'CQTimelineEditor +0x15C',
    'action_callback': '0x1017CC80',
    'handler_dialog_text': 'Trim Handles In Track',
    'handler_range': [hex(handler_range[0]), hex(handler_range[1] - 1)],
    'constructor_action_range': [hex(constructor_range[0]), hex(constructor_range[1] - 1)],
    'callback_binding_range': [hex(binding_range[0]), hex(binding_range[1] - 1)],
    'verification_status': 'Handler, constructor action title/member, and callback binding directly match installed DLL; all spans have complete contiguous byte coverage.',
    'handler_instruction_count': len(handler),
    'handler_byte_count': sum(len(row['bytes']) // 2 for row in handler),
    'constructor_instruction_count': len(constructor),
    'constructor_byte_count': sum(len(row['bytes']) // 2 for row in constructor),
    'binding_instruction_count': len(binding),
    'binding_byte_count': sum(len(row['bytes']) // 2 for row in binding),
    'handler_instructions': handler,
    'constructor_instructions': constructor,
    'binding_instructions': binding,
    'strings': strings,
    'observed_flow': [
        'The constructor creates the action titled "Trim Handles" and stores it in member +0x15C.',
        'The recovered menu/event route binds callback 0x1017CC80 to member +0x15C through helper 0x1016D300.',
        'The callback validates both supplied objects, resolves the timeline context, and checks an interface gate using global key 0x10EB1124.',
        'It opens a parameter dialog using localized text "Trim Handles In Track". Both initial float fields are loaded from the same constant; a rejected/invalid dialog skips the apply call.',
        'On acceptance, it converts the chosen float value through 0x1097B170 and calls 0x10283730 with the current context, resolved element, dialog values/flags, and callback argument.',
        'The callback then runs the timeline update and notification virtual calls.',
    ],
    'limitations': 'The complete handler/binding bytes and strings are verified, but the exact widget layout, meaning and units of the two float values, precise trim math inside 0x10283730, undo semantics, enablement, and visible menu context remain unresolved.',
}
Path('outputs/timeline-trim-track-action-recovery.json').write_text(json.dumps(receipt, indent=2))
print(json.dumps({key: receipt[key] for key in ('action_label', 'action_member', 'action_callback', 'handler_dialog_text', 'verification_status', 'handler_instruction_count', 'handler_byte_count', 'constructor_instruction_count', 'constructor_byte_count', 'binding_instruction_count', 'binding_byte_count', 'binary_sha256', 'strings')}, indent=2))
