"""Verify direct gamma calls and one three-channel HDR-preserving path."""
from pathlib import Path
import json, re, struct
import audit_installed_gamma_conversion as gamma

rows = []
pending = None
with Path('work/shaderapidx9-disassembly.txt').open() as source:
    for line in source:
        match = re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)', line)
        if match:
            pending = {'address': int(match[1],16), 'bytes': bytes.fromhex(match[2]).hex(), 'instruction': match[3].strip()}
            rows.append(pending)
        elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):
            pending['bytes'] += bytes.fromhex(line).hex()
calls = [row for row in rows if re.fullmatch(r'call\s+100D4EF0',row['instruction'])]
path = [row for row in rows if 0x10021d90 <= row['address'] < 0x10021e66]
for row in calls + path:
    raw = bytes.fromhex(row['bytes'])
    offset = gamma.image.offset(row['address'])
    assert gamma.image.b[offset:offset+len(raw)] == raw
for row in calls:
    raw = bytes.fromhex(row['bytes'])
    assert raw[0] == 0xe8 and len(raw) == 5
    assert row['address'] + 5 + struct.unpack('<i',raw[1:])[0] == 0x100d4ef0
one_address = 0x100fa5f8
offset = gamma.image.offset(one_address)
one_bytes = gamma.image.b[offset:offset+4]
assert struct.unpack('<f',one_bytes)[0] == 1.0
assert len(calls) == 12
assert [row['address'] for row in path if row in calls] == [0x10021e06,0x10021e33,0x10021e58]
report = {
 'installed_shaderapi_sha256': gamma.digest, 'verified_direct_calls': calls,
 'verified_three_channel_path': path,
 'comparison_constant': {'address':hex(one_address),'bytes':one_bytes.hex(),'float':1.0},
 'finding':'The installed shader API contains a three-component path that skips conversion above float 1.0 and otherwise calls the recovered gamma lookup; this supports the SDK HDR-preserving conversion policy.',
 'limits':'Static verified instructions, not native execution. This path has not been traced back to a named selfillum material parameter or selected shader. Direct-call inventory excludes indirect calls and other modules. x87/SSE rounding-mode parity remains unresolved.'
}
Path('outputs/installed-gamma-callers-audit.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'PASS: {len(calls)} direct calls and {len(path)} path instructions verified; HDR threshold=1.0')
