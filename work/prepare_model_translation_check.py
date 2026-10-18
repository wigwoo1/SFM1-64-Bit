from pathlib import Path
import struct
Path('work/diagnostic-model-translation-v1.bin').write_bytes(struct.pack('<16f',1,0,0,5,0,1,0,0,0,0,1,0,0,0,0,1))
