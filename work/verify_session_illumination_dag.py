"""Read-only session audit plus pinned IFM byte evidence; no runtime parity claim."""
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile
from pe_image import Image

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'outputs/session-illumination-dag-20261015'
GAME = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
IFM = GAME / 'bin/tools/ifm.dll'
SESSION = GAME / 'tf_movies/elements/sessions/mtt_soldier/mtt_soldier.dmx'
IFM_SHA = '8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4'
SESSION_SHA = '5a08cf8eeb086586f05dc856fb5469dd238fde90315ecf17c2dd4ab5f0742aee'
RANGES = [
    ('typed_DAG_attribute_getter', 0x10130ed0, 0x10130f15),
    ('present_attribute_selection_and_absent_attribute_fallback', 0x10243967, 0x10243a57),
    ('draw_record_lighting_pointer', 0x10243b61, 0x10243b73),
    ('DAG_world_position', 0x104c5900, 0x104c5975),
]


def require(value, message):
    if not value:
        raise RuntimeError(message)


def sha(path):
    return hashlib.file_digest(path.open('rb'), 'sha256').hexdigest()


def main():
    require(sha(IFM) == IFM_SHA, 'Original IFM changed')
    require(sha(SESSION) == SESSION_SHA, 'Benchmark session changed')
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    image = Image(IFM)
    require(image.string(0x10abd670) == 'illumPositionDag', 'Attribute string changed')
    rows = []
    pending = None
    # Stream the large disassembly rather than loading it all into RAM.
    with (ROOT / 'work/ifm-disassembly.txt').open() as source:
        for line in source:
            match = re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)', line)
            if match:
                pending = None
                address = int(match[1], 16)
                if any(start <= address < end for _, start, end in RANGES):
                    pending = dict(address=address, bytes=bytes.fromhex(match[2]).hex(),
                                   instruction=match[3].strip())
                    rows.append(pending)
            elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+', line):
                pending['bytes'] += bytes.fromhex(line).hex()
    checked = []
    for label, start, end in RANGES:
        selected = [row for row in rows if start <= row['address'] < end]
        cursor = start
        for row in selected:
            require(row['address'] == cursor, 'Disassembly gap: ' + label)
            raw = bytes.fromhex(row['bytes'])
            offset = image.offset(cursor)
            require(image.b[offset:offset + len(raw)] == raw, 'Stale bytes: ' + label)
            cursor += len(raw)
        require(cursor == end, 'Incomplete range: ' + label)
        checked.append(dict(name=label, start=hex(start), end_exclusive=hex(end),
                            byte_count=end-start, instructions=selected))
    executables = []
    logs = {}
    for name, args in [('dag_safety_x64', []), ('dag_audit_x64', [str(SESSION)])]:
        exe = OUT / (name + '.exe')
        binary = exe.read_bytes()
        pe = struct.unpack_from('<I', binary, 60)[0]
        require(binary[pe:pe+4] == b'PE\0\0' and
                struct.unpack_from('<H', binary, pe+4)[0] == 0x8664,
                'AMD64 executable required')
        result = subprocess.run([str(exe), *args], capture_output=True, text=True, timeout=60)
        logs[name] = result.stdout + result.stderr
        (run / (name + '.log')).write_text(logs[name])
        require(result.returncode == 0 and result.stdout.startswith('PASS' if not args else 'tick='),
                name + ' failed: ' + logs[name])
        executables.append(dict(executable=str(exe), sha256=sha(exe), machine='AMD64'))
    audit = logs['dag_audit_x64']
    require('models=3\n' in audit and 'PASS selected=1 missing=2 ' in audit,
            'Pinned benchmark coverage changed')
    require('model=13817 asset=models/weapons/w_models/w_flamethrower.mdl attribute_present=1 first_child_attempted=0 dag=13818 ' in audit,
            'Pinned explicit DAG binding changed')
    require('native_session_binding=UNVERIFIED renderer_changed=0' in audit,
            'Audit must retain its verification limitations')
    require(sha(IFM) == IFM_SHA and sha(SESSION) == SESSION_SHA, 'Original inputs changed')
    receipt = dict(status='PARTIALLY VERIFIED', checks='PASS', static_byte_count=sum(b-a for _, a, b in RANGES),
                   original_inputs=[dict(path=str(IFM), sha256=IFM_SHA),
                                    dict(path=str(SESSION), sha256=SESSION_SHA)],
                   static_evidence=checked, executables=executables, logs=logs,
                   limits=['Static IFM bytes checked; original IFM routines not executed in this task',
                           'One evaluated benchmark shot at tick 50000; world position is an owned graph diagnostic',
                           'Live native session position and full renderer behavior remain UNVERIFIED',
                           'Audit accepts exact DmeDag only; general ancestry and external DAG scope unresolved',
                           'Absent-attribute fallback deliberately not implemented',
                           'No editor integration, renderer change, original writes, or editor rebuild'])
    (run / 'results.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(dict(status=receipt['status'], checks='PASS', static_byte_count=receipt['static_byte_count'],
                          selected=1, missing=2, receipt=str(run / 'results.json'))))


if __name__ == '__main__':
    main()
