from pathlib import Path
import re

ranges = [(0x102aed40, 0x102aeec0), (0x102af250, 0x102af2da),
          (0x102b26d0, 0x102b2750), (0x102b7870, 0x102b7910),
          (0x102b7980, 0x102b7a20), (0x102b7f20, 0x102b8060)]
lines = []
with Path('work/ifm-disassembly.txt').open() as source:
    for line in source:
        match = re.match(r'  ([0-9A-F]{8}):', line)
        if match:
            address = int(match[1], 16)
            if address >= ranges[-1][1]:
                break
            if any(start <= address < end for start, end in ranges):
                lines.append(line)
Path('outputs/sfm64-ifm-duration-producers.txt').write_text(''.join(lines))
print(''.join(lines))
