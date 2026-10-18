import hashlib
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, 'work')
from pe_image import Image

BINARY = r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll'
DISASM = Path('work/ifm-disassembly.txt')
image = Image(BINARY)


def check(start, end):
    rows, current, active = [], None, False
    for line in DISASM.open(errors='replace'):
        match = re.match(r'^\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)', line)
        if match:
            va = int(match.group(1), 16)
            if va < start:
                continue
            active = True
            if current:
                rows.append(current)
            if va >= end:
                current = None
                break
            current = [va, bytes.fromhex(match.group(2))]
        elif active and current and re.match(r'^\s{10,}[0-9A-F]{2}(?:\s+[0-9A-F]{2})*\s*$', line):
            current[1] += bytes.fromhex(line.strip())

    cursor, total, mismatches = start, 0, []
    for va, expected in rows:
        actual = image.b[image.offset(va):image.offset(va) + len(expected)]
        if va != cursor or actual != expected:
            mismatches.append({'va': hex(va), 'expected_va': hex(cursor), 'bytes_match': actual == expected})
        cursor = va + len(actual)
        total += len(actual)
    if cursor != end:
        mismatches.append({'expected_end': hex(end), 'actual_end': hex(cursor)})
    return {'start': hex(start), 'end_exclusive': hex(end), 'bytes': total,
            'all_match': not mismatches, 'mismatches': mismatches}


checks = {
    'dialog_constructor': check(0x10124CE0, 0x10124D5A),
    'selected_model_path_extractor': check(0x10124F80, 0x1012504F),
    'accepted_callback': check(0x10125110, 0x10125152),
    'rejected_callback': check(0x10125050, 0x10125102),
    'create_animation_set_and_model_path': check(0x10223950, 0x10223AF9),
    'dialog_and_helper_signal_connections': check(0x10125160, 0x1012522B),
    'qt_meta_dispatch': check(0x10075280, 0x100752A8),
    'slot_id_dispatch': check(0x10075310, 0x10075368),
}

sha = hashlib.sha256(image.b).hexdigest()
assert sha == '8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4'
assert all(result['all_match'] for result in checks.values()), checks
assert image.string(0x10A9D3CC) == 'CQMDLPickerDialog'
assert image.string(0x10A9D460) == 'CQMDLPickerDialogHelper'
assert image.string(0x10ABA430) == 'Select .MDL file'
assert image.string(0x10ABA598) == 'models\\%s'
assert image.string(0x10ABA5A4) == '(null)'
assert image.string(0x10ACECA4) == 'CreateAnimationSetAndModel'
assert image.string(0x10AA7F00) == 'models'
assert image.string(0x10A8A8AC) == '%s%d'
assert image.string(0x10ABA5AC) == '1SelectModelAndCreateAnimationSetWithDialogCanceled()'
assert image.string(0x10ABA5E4) == '1SelectModelAndCreateAnimationSetWithDialogAccepted()'
assert image.string(0x10AA5CCC) == '2accepted()'
assert image.string(0x10AA7BD4) == '2rejected()'
assert image.word(0x10124D09) == 0x10ABA464
assert image.word(0x10124D10) == 0x10ABA448
assert image.word(0x10ABA444) == 0x10E23A0C
assert image.word(0x10ABA460) == 0x10E239B0
assert image.string(image.word(0x10E239B0 + 12) + 8) == '.?AVCQMDLPickerDialog@@'
assert image.string(image.word(0x10E23A0C + 12) + 8) == '.?AVCQMDLPickerDialog@@'

receipt = {
    'binary': BINARY,
    'sha256': sha,
    'class': 'CQMDLPickerDialog',
    'class_helper': 'CQMDLPickerDialogHelper',
    'dialog_title': 'Select .MDL file',
    'constructor': '0x10124CE0 installs the two verified class vtables and assigns the dialog title',
    'signal_wiring': {
        'sender': 'CQMDLPickerDialog',
        'accepted_signal': 'accepted()',
        'accepted_helper_slot': 'SelectModelAndCreateAnimationSetWithDialogAccepted()',
        'rejected_signal': 'rejected()',
        'rejected_helper_slot': 'SelectModelAndCreateAnimationSetWithDialogCanceled()',
        'connection_type_argument': 0,
        'helper_fields': {'callback_context_a': 'this+0x08', 'callback_context_b': 'this+0x0C', 'dialog': 'this+0x10'},
    },
    'selected_model_path': {
        'extractor': '0x10124F80 reads the picker UI selection and formats a nonempty value with `models\\%s`',
        'fallback': '(null)',
        'callback_followup': 'the rejected/canceled helper slot calls the extractor and passes a nonempty result to 0x10223950; the callee reads models-root content, derives a `%s%d` name, and routes through an application handler labeled `CreateAnimationSetAndModel`',
        'accepted_followup': 'the accepted helper slot dispatches virtual method 1 through its stored dialog/context field; that branch does not call 0x10223950',
        'semantic_limit': 'exact callback argument meaning, model-load success/failure effects, animation-set contents and whether the picker emits accepted or rejected in each user gesture remain unverified',
    },
    'dispatch': {
        'local_method_id_0': 'rejected/canceled helper callback at 0x10125050',
        'local_method_id_1': 'accepted helper callback at 0x10125110',
        'route_evidence': 'the local Qt meta-call dispatcher at 0x10075280 maps result 0 to canceled and result 1 to accepted',
    },
    'scope': 'confirms class identity, title, constructor, signal wiring, helper dispatch, selected-path extraction and the canceled-path downstream CreateAnimationSetAndModel route; browser filtering internals, exact button/signal semantics, animation-set result and model-load effects remain open',
    'checks': checks,
    'all_match': True,
}
Path('outputs/mdl-picker-dialog-wiring-verification.json').write_text(json.dumps(receipt, indent=2) + '\n')
print(json.dumps({'sha256': sha,
                  'checked_bytes': sum(item['bytes'] for item in checks.values()),
                  'ranges': len(checks),
                  'all_match': True}, indent=2))
