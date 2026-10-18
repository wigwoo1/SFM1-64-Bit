from pathlib import Path
from pe_image import Image
import struct,json,hashlib
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll');w=im.word(0x104083f4)
report=dict(binary_sha256=hashlib.sha256(im.b).hexdigest(),clock_base='0x103ff7ac',tick_update='0x101bc4c0',input_signed_tick_offset='0x10',previous_tick_object_offset='0x4a28',interval_address='0x104083f4',interval_initial_bits=hex(w),interval_initial_float=struct.unpack('<f',struct.pack('<I',w))[0],current='SSE CVTSI2SS signed input tick then MULSS interval ->103ff7b8',duration='32-bit SUB of prior tick, then CVTSI2SS signed difference and MULSS interval ->103ff7bc',external_duration_setter='0x101e57b0',limits=['static recovery','interval runtime value/writers and previous-tick update unresolved','SFM caller of tick-update interface not yet established'])
Path('outputs/sfm64-engine-animation-clock-update-recovery.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
