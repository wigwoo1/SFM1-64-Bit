import argparse
import hashlib
import json
import re
from pathlib import Path

source = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
start, end = 0x1016C590, 0x1016C7D5
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
    raw = bytes.fromhex(match[2])
    spans.append({'va': hex(va), 'bytes': raw.hex(), 'instruction': match[3].strip()})

cursor = start
for row in spans:
    assert int(row['va'], 16) == cursor, (hex(cursor), row['va'])
    cursor += len(row['bytes']) // 2
assert cursor == end, (hex(cursor), hex(end))

string_rows = {}
for line in Path('work/ifm-strings.txt').open():
    match = re.match(r'([0-9A-F]{8})\s+(.*)', line.rstrip())
    if match:
        string_rows[int(match[1], 16)] = match[2]
strings = {hex(va): string_rows[va] for va in (0x10AC33F0, 0x10AC33F8, 0x10AC3400, 0x10ABAFB4)}
call_targets = []
for row in spans:
    match = re.search(r'\b(?:call|jmp)\s+(?:near ptr\s+)?([0-9A-F]{8})\b', row['instruction'], re.IGNORECASE)
    if match:
        call_targets.append({'va': row['va'], 'target': '0x' + match[1].lower(), 'instruction': row['instruction']})
args = argparse.ArgumentParser()
args.add_argument('--verify-installed', action='store_true', help='Compare rows with the installed DLL; can be slow if the library stalls.')
verify_installed = args.parse_args().verify_installed
binary_hash = None
if verify_installed:
    from pe_image import Image
    image = Image(source)
    for row in spans:
        offset = image.offset(int(row['va'], 16))
        assert image.b[offset:offset + len(bytes.fromhex(row['bytes']))] == bytes.fromhex(row['bytes']), row['va']
    binary_hash = hashlib.sha256(image.b).hexdigest()
receipt = {
    'binary': str(source),
    'binary_sha256': binary_hash,
    'action': 'Add Film Clip',
    'action_member': 'CQTimelineEditor +0x12C',
    'action_callback': '0x1016C590',
    'callback_range': [hex(start), hex(end - 1)],
    'verification_status': 'Directly byte-verified against installed DLL.' if verify_installed else 'Saved-disassembly-only; direct byte verification of this handler is pending.',
    'disassembly_instructions': spans,
    'disassembly_instruction_count': len(spans),
    'disassembly_byte_count': sum(len(row['bytes']) // 2 for row in spans),
    'strings': strings,
    'direct_call_and_jump_targets': call_targets,
    'observed_flow': [
        'The callback reads global timeline context member +0x344 and exits if the context or resolver result is absent.',
        'It passes the text “Add New Film Clip To Sequence” through sequence/timeline virtual calls.',
        'It references default names “camera1” and “shot1” while resolving sequence and clip data.',
        'It creates and configures a data-model element with “Default Set”, duration/timing values, associates it with the selected track, and runs timeline update/notification calls.',
    ],
    'saved_disassembly_helper_notes': [
        '0x1008C0E0 is a two-instruction accessor that returns the pointer stored at receiver +0x68.',
        '0x104D31C0 reads a pointer from receiver +0x5C at the requested index, resolves it through a global service virtual at +0x48, then returns it only if its virtual at +0x10 accepts the global interface key at 0x10EB10A4.',
        '0x104BBA80 resolves a receiver field at +0x78 through the global service virtual at +0x48; on success it writes the sum of resolved-object fields +0x5C and +0x64 to its output argument, otherwise writes zero.',
        'These helpers support a collection-backed, interface-gated timing lookup interpretation; concrete class names, field meanings, and transaction semantics are not established by this snapshot.',
    ],
    'limitations': ('The handler instruction bytes directly match installed ifm.dll.' if verify_installed else 'The handler body is supported only by the saved disassembly; direct installed-file comparison is pending.') + ' The helper notes are saved-disassembly-only. Called helper and transaction virtual semantics, exact prompt presentation, clip class/runtime type, and live menu behavior remain unresolved. No UI runtime parity claim.',
}
Path('outputs/timeline-add-film-clip-handler-recovery.json').write_text(json.dumps(receipt, indent=2))

menu_path = Path('outputs/timeline-trackgroup-menu-preparation-recovery.json')
menu = json.loads(menu_path.read_text())
menu['action_identities']['member_12c']['callback'] = '0x1016C590'
verification_note = ('The handler body was directly byte-verified against installed ifm.dll SHA-256 ' + binary_hash + '.'
                     if verify_installed else 'The handler body is saved-disassembly-only pending direct verification.')
menu['action_identities']['member_12c']['evidence'] = ('Constructor registration at 0x10177553-0x101775A4 uses localized title 0x10AC4A50 and stores the QAction in member +0x12C. '
    'The constructor binds member function 0x1016C590 to the action. ' + verification_note)
menu_path.write_text(json.dumps(menu, indent=2))
print(json.dumps({
    'action': receipt['action'],
    'action_callback': receipt['action_callback'],
    'verification_status': receipt['verification_status'],
    'disassembly_instruction_count': receipt['disassembly_instruction_count'],
    'disassembly_byte_count': receipt['disassembly_byte_count'],
    'binary_sha256': receipt['binary_sha256'],
    'strings': receipt['strings'],
    'direct_call_and_jump_targets': receipt['direct_call_and_jump_targets'],
}, indent=2))
