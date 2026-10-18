"""Verify the nearby registration before treating its getter as a queue alias."""
from pathlib import Path
import hashlib
import json
import re
from pe_image import Image

source = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
image = Image(source)
digest = hashlib.sha256(image.b).hexdigest()
assert digest == '07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
regions = [(0x10002cb0, 0x10002cb6), (0x10002cd0, 0x10002ce0),
           (0x100f9170, 0x100f9185), (0x100f9190, 0x100f91bc)]
instructions = []
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
    match = re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)', line)
    if not match:
        continue
    address = int(match[1], 16)
    if not any(begin <= address < end for begin, end in regions):
        continue
    raw = bytes.fromhex(match[2])
    offset = image.offset(address)
    assert image.b[offset:offset + len(raw)] == raw
    instructions.append({'address': hex(address), 'bytes': raw.hex(), 'instruction': match[3].strip()})
assert instructions
strings = {hex(address): image.string(address) for address in
           [0x100fa41c, 0x100fa430, 0x100fa444, 0x100fa3c8]}
assert strings['0x100fa41c'] == 'VBAllocTracker001'
assert strings['0x100fa430'] == 'mem_dumpvballocs'
assert strings['0x100fa3c8'] == 'ENABLE_VB_ALLOC_TRACKER must be 1 to enable VB mem alloc tracking\n'
assert image.b[image.offset(0x10002cb0):image.offset(0x10002cb0)+6] == bytes.fromhex('b848d01610c3')
report = {'source': str(source), 'sha256': digest,
          'verified_instruction_prefixes': instructions, 'registration_strings': strings,
          'getter': '0x10002cb0', 'returned_address': '0x1016d048',
          'conclusion': 'The nearby getter is registered as VBAllocTracker001. It is not evidence of an alias to the device command ring.',
          'diagnostic_behavior': 'The registered dump command reports that ENABLE_VB_ALLOC_TRACKER must be enabled.',
          'consumer_identified': False,
          'next_action': 'Trace command-mode enable/setup and indirect consumer dispatch independently of this allocation-tracker registration.',
          'limits': 'Static registration and displayed instruction-prefix verification only; does not establish runtime queue activation or consumer absence.'}
Path('outputs/sfm64-queue-neighbor-registration.json').write_text(json.dumps(report, indent=2)+'\n')
print(f'verified_registration_instruction_prefixes={len(instructions)} queue_alias_evidence=0')
