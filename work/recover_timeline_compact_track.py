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

handler_range = (0x1017C8C0, 0x1017C9F9)
constructor_range = (0x101775C4, 0x101775F0)
binding_ranges = ((0x1017333F, 0x10173357), (0x101735FD, 0x10173615))
helper_ranges = {
    'collection_count_accessor': (0x1008C0E0, 0x1008C0E4),
    'resolved_timing_sum_helper': (0x104BBA80, 0x104BBACB),
    'resolved_value_setter_wrapper': (0x104BBB60, 0x104BBB91),
    'typed_attribute_write_helper': (0x10366370, 0x10366474),
}
handler = extract(*handler_range)
constructor = extract(*constructor_range)
bindings = [extract(*r) for r in binding_ranges]
helpers = {name: extract(*r) for name, r in helper_ranges.items()}
image = Image(source)
all_spans = handler + constructor + [row for binding in bindings for row in binding] + [row for helper in helpers.values() for row in helper]
for row in all_spans:
    offset = image.offset(int(row['va'], 16))
    raw = bytes.fromhex(row['bytes'])
    assert image.b[offset:offset + len(raw)] == raw, row['va']
binary_hash = hashlib.sha256(image.b).hexdigest()

strings = {}
for line in Path('work/ifm-strings.txt').open():
    match = re.match(r'([0-9A-F]{8})\s+(.*)', line.rstrip())
    if match and int(match[1], 16) in (0x10AC4A30, 0x10AC5550):
        strings[hex(int(match[1], 16))] = match[2]

receipt = {
    'binary': str(source),
    'binary_sha256': binary_hash,
    'action_label': 'Compact Track',
    'action_member': 'CQTimelineEditor +0x124',
    'action_callback': '0x1017C8C0',
    'handler_dialog_text': 'Compact clips',
    'handler_range': [hex(handler_range[0]), hex(handler_range[1] - 1)],
    'constructor_label_binding_range': [hex(constructor_range[0]), hex(constructor_range[1] - 1)],
    'context_menu_callback_binding_ranges': [[hex(a), hex(b - 1)] for a, b in binding_ranges],
    'verification_status': 'Handler, constructor label/member registration, and both callback-binding spans directly match installed DLL; all spans have complete contiguous byte coverage.',
    'handler_instruction_count': len(handler),
    'handler_byte_count': sum(len(row['bytes']) // 2 for row in handler),
    'constructor_instruction_count': len(constructor),
    'constructor_byte_count': sum(len(row['bytes']) // 2 for row in constructor),
    'binding_instruction_counts': [len(b) for b in bindings],
    'binding_byte_counts': [sum(len(row['bytes']) // 2 for row in b) for b in bindings],
    'helper_ranges': {name: [hex(r[0]), hex(r[1] - 1)] for name, r in helper_ranges.items()},
    'helper_instruction_counts': {name: len(rows) for name, rows in helpers.items()},
    'helper_byte_counts': {name: sum(len(row['bytes']) // 2 for row in rows) for name, rows in helpers.items()},
    'helper_instructions': helpers,
    'handler_instructions': handler,
    'constructor_instructions': constructor,
    'binding_instructions': bindings,
    'strings': strings,
    'observed_flow': [
        'The constructor stores the action titled "Compact Track" in member +0x124.',
        'Two event/menu wiring paths pass callback 0x1017C8C0 and action member +0x124 to callback helper 0x1016CCC0.',
        'The handler validates the supplied model element and opens a dialog using the localized text "Compact clips".',
        'It reads the collection count through helper 0x1008C0E0. A count of zero or one takes the no-reorder/update path.',
        'For larger collections, the saved control flow reads the first element timing, then loops from element index 1: it passes the previously computed time value to a setter-like helper for the current element, reads that element timing, and carries the result forward. It then dispatches the timeline update/notification path.',
        'Directly verified helper 0x104BBA80 resolves the timing object and writes the sum of its +0x5C and +0x64 fields to an output; helper 0x104BBB60 resolves another object and writes the supplied value through the typed data-model attribute writer 0x10366370 at resolved offset +0x60.',
        'This loop is consistent with moving each later clip to the accumulated end of the preceding clip, but the concrete model/attribute names and time units are not identified.',
    ],
    'limitations': 'Direct byte equality proves the code and registration ranges, not live UI presentation. Exact semantics of the called timing setter/getter helpers, whether “Compact clips” is a title or another dialog string, action enablement, and undo/transaction behavior remain unverified.',
}
Path('outputs/timeline-compact-track-action-recovery.json').write_text(json.dumps(receipt, indent=2))
print(json.dumps({key: receipt[key] for key in ('action_label', 'action_member', 'action_callback', 'handler_dialog_text', 'verification_status', 'handler_instruction_count', 'handler_byte_count', 'constructor_instruction_count', 'constructor_byte_count', 'binding_instruction_counts', 'binding_byte_counts', 'helper_instruction_counts', 'helper_byte_counts', 'binary_sha256', 'strings')}, indent=2))
