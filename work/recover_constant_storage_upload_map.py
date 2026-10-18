"""Record upload boundaries from the previously byte-verified restore body."""
from pathlib import Path
import json

source = json.loads(Path('outputs/sfm64-owner-reset-cleanup-recovery.json').read_text())
rows = {int(row['address'], 16): row for row in source['instructions']}

def require(address, instruction):
    assert rows[address]['instruction'] == instruction, (hex(address), rows[address])

phases = []
for start, end, count, buffer, cache, opcode, op_address, call, slot_address, slot in [
    (0x10015d4d, 0x10015e51, 0x528, 0x2864, 0x19ec, 7, 0x10015dd1, 0x10015e2a, 0x10015e24, 109),
    (0x1001605f, 0x10016161, 0x534, 0x2858, 0x19e0, 10, 0x100160e4, 0x1001613d, 0x10016134, 94),
]:
    require(op_address, f'mov         dword ptr [eax],{opcode:02X}h' if opcode >= 10 else f'mov         dword ptr [eax],{opcode}')
    expected = 'eax' if slot == 109 else 'ecx'
    require(call, f'call        {expected}')
    require(slot_address, f'mov         {expected},dword ptr [{"edx" if slot == 109 else "eax"}+{slot*4:08X}h]')
    phases.append(dict(start=hex(start), end_exclusive=hex(end), count_offset=hex(count), staging_pointer_offset=hex(buffer), cache_pointer_offset=hex(cache), queued_opcode=opcode, direct_device_slot=slot, queue_words='uint32(count * 4 + 3)', payload_bytes='uint32(count << 4)', behavior='Zero staging float4 entries for signed-positive count; submit from register zero via queue or device; copy staging to cache and staging through 100df030. Copy helper semantics not independently verified.', evidence=[rows[a] for a in sorted(rows) if start <= a < end]))

submissions = []
for call, lookup, slot, count, buffer, zero_skip in [
    (0x10015e9b, 0x10015e84, 43, 0x52c, 0x2868, True),
    (0x10015eed, 0x10015ed6, 44, 0x530, 0x286c, True),
    (0x100161bb, 0x100161aa, 41, 0x538, 0x285c, False),
    (0x10016217, 0x10016206, 42, 0x53c, 0x2860, False),
]:
    register = 'eax' if zero_skip else 'edx'
    require(call, f'call        {register}')
    require(lookup, f'mov         {register},dword ptr [{register}+{slot*4:08X}h]')
    submissions.append(dict(call=hex(call), owner_subobject_offset='0x2d0', vtable_slot=slot, count_offset=hex(count), staging_pointer_offset=hex(buffer), skip_when_count_zero=zero_skip, arguments='start=0, staging pointer, count, flag=1', interface_method_name='unresolved'))

require(0x10015e64, 'je          10015E9D')
require(0x10015eae, 'je          10015EEF')
out = dict(source_sha256=source['sha256'], float_upload_phases=phases, subobject_submissions=submissions, tail_calls=[rows[a] for a in sorted(rows) if 0x10016219 <= a < 0x10016248], limits='Static verified-instruction mapping only. No whole-helper execution, actual GPU binding, copy helper equivalence, or owned allocation failure handling.')
Path('outputs/sfm64-constant-storage-upload-map.json').write_text(json.dumps(out, indent=2) + '\n')
print('constant_upload_phases=2 subobject_submissions=4 verified')
