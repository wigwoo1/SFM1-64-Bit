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

handler_range = (0x1017C7B0, 0x1017C8B7)
binding_range = (0x1017307A, 0x101730A0)
index_lookup_range = (0x104BE8A0, 0x104BE916)
array_access_range = (0x104BB640, 0x104BB64C)
handler = extract(*handler_range)
binding = extract(*binding_range)
index_lookup = extract(*index_lookup_range)
array_access = extract(*array_access_range)
image = Image(source)
for row in handler + binding + index_lookup + array_access:
    offset = image.offset(int(row['va'], 16))
    raw = bytes.fromhex(row['bytes'])
    assert image.b[offset:offset + len(raw)] == raw, row['va']
binary_hash = hashlib.sha256(image.b).hexdigest()

strings = {}
for line in Path('work/ifm-strings.txt').open():
    match = re.match(r'([0-9A-F]{8})\s+(.*)', line.rstrip())
    if match and int(match[1], 16) in (0x10AC553C,):
        strings[hex(int(match[1], 16))] = match[2]

receipt = {
    'binary': str(source),
    'binary_sha256': binary_hash,
    'action': 'CQTimelineEditor member +0x10C callback',
    'handler_dialog_title': 'Remove Track Group',
    'menu_action_label_status': 'Unresolved; the recovered string is used by the handler dialog, not proven to be the QAction label.',
    'action_member': 'CQTimelineEditor +0x10C (callback binding observed; runtime owner/context not confirmed)',
    'action_callback': '0x1017C7B0',
    'callback_range': [hex(handler_range[0]), hex(handler_range[1] - 1)],
    'binding_range': [hex(binding_range[0]), hex(binding_range[1] - 1)],
    'verification_status': 'Handler and binding instruction bytes directly match installed DLL; complete contiguous spans asserted.',
    'disassembly_instructions': handler,
    'disassembly_instruction_count': len(handler),
    'disassembly_byte_count': sum(len(row['bytes']) // 2 for row in handler),
    'binding_instructions': binding,
    'binding_instruction_count': len(binding),
    'binding_byte_count': sum(len(row['bytes']) // 2 for row in binding),
    'index_lookup_helper_range': [hex(index_lookup_range[0]), hex(index_lookup_range[1] - 1)],
    'index_lookup_helper_instruction_count': len(index_lookup),
    'index_lookup_helper_byte_count': sum(len(row['bytes']) // 2 for row in index_lookup),
    'index_lookup_helper_instructions': index_lookup,
    'array_access_wrapper_range': [hex(array_access_range[0]), hex(array_access_range[1] - 1)],
    'array_access_wrapper_instruction_count': len(array_access),
    'array_access_wrapper_byte_count': sum(len(row['bytes']) // 2 for row in array_access),
    'array_access_wrapper_instructions': array_access,
    'strings': strings,
    'observed_flow': [
        'The handler reads global timeline context member +0x344 and returns false if the context or resolved element is unavailable.',
        'It resolves an input-associated index, opens a dialog using localized text "Remove Track Group", and exits without mutation when dialog acceptance fails.',
        'The directly verified 0x104BE8A0 helper walks the current object’s collection count at +0x68 and pointers at +0x5C, resolves each candidate through the global service, checks interface key 0x10EB30FC, and returns the zero-based index matching the requested resolved element (or -1).',
        'On acceptance it resolves the target element/index, invokes timeline operations, and conditionally dispatches a follow-up update using dialog state.',
        'The directly verified 0x104BB640 wrapper offsets its receiver to +0x5C before delegating indexed access to 0x103885E0; the delegated routine is not included in this receipt.',
        'The verified binding site passes 0x1017C7B0 to callback helper 0x1016CC40 for the action stored at receiver member +0x10C.',
    ],
    'limitations': 'Static code and string evidence only. Exact prompt/widget contents, meaning of the two dialog-state bytes, action enablement, undo/transaction semantics, menu presentation, and the owning UI context remain unresolved.',
}
Path('outputs/timeline-remove-track-group-handler-recovery.json').write_text(json.dumps(receipt, indent=2))
print(json.dumps({key: receipt[key] for key in ('action', 'handler_dialog_title', 'action_callback', 'verification_status', 'disassembly_instruction_count', 'disassembly_byte_count', 'binding_instruction_count', 'binding_byte_count', 'binary_sha256', 'strings')}, indent=2))
