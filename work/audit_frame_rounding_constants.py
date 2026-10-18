from pe_image import Image
from pathlib import Path
import struct,json
im=Image('D:/SteamLibrary/steamapps/common/SourceFilmmaker/game/sdktools/python/2.7/win32/Lib/site-packages/vs/valve_pyd.pyd')
report={'mode_dispatch':[hex(im.word(0x101571e4+4*n)) for n in range(4)],'constants':{hex(va):{'bytes':im.b[im.offset(va):im.offset(va)+8].hex(),'double':struct.unpack_from('<d',im.b,im.offset(va))[0]} for va in (0x10353c70,0x10353c68,0x101e63f0,0x101e6640)}}
Path('outputs/sfm64-native-frame-rounding-constants.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
lines=Path('work/native-time-1015EF10.txt').read_text().splitlines();print('\n'.join(lines[65:]))
