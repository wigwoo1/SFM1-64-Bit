from pe_image import Image
from pathlib import Path
import struct,json
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
print('initial interval',hex(im.word(0x10357388)),struct.unpack('<f',struct.pack('<I',im.word(0x10357388)))[0]);print('rejection message',im.string(0x1035eef4))
needle=struct.pack('<I',0x101bc4c0);o=im.b.find(needle)
for r,s,raw in im.sections:
 if raw<=o<raw+s:
  va=im.base+r+o-raw;print('function pointer',hex(va));print([hex(im.word(va+j*4)) for j in range(-3,6)])
report=dict(default_interval_bits=hex(im.word(0x10357388)),default_interval_float=struct.unpack('<f',struct.pack('<I',im.word(0x10357388)))[0],external_interval_record_offset='0x34',interval_change_guard_state_address='0x1060d4ec',guard_state=3,guard='ordered unequal or unordered interval comparison rejects; ordered equal retains existing interval',diagnostic=im.string(0x1035eef4),limits='Static recovery; previous-tick offset must be interpreted relative to actual adjusted this pointer, not unrelated same-offset accesses.')
Path('outputs/sfm64-engine-animation-interval-recovery.json').write_text(json.dumps(report,indent=2))
