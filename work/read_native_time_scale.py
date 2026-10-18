from pathlib import Path
from pe_image import Image
import struct,json
path=Path('D:/SteamLibrary/steamapps/common/SourceFilmmaker/game/sdktools/python/2.7/win32/Lib/site-packages/vs/valve_pyd.pyd');im=Image(path);bits=im.word(0x10353c78);value=struct.unpack('<f',struct.pack('<I',bits))[0]
report={'method':'DmeTime_t_GetSeconds','wrapper_va':'0x1015faf0','conversion_va':'0x1015fb64','factor_va':'0x10353c78','factor_bits':hex(bits),'factor_float':value,'arithmetic':'signed int32 to float32, multiply by factor, widen result to float64 for Python'}
Path('outputs/sfm64-native-time-scale-recovery.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
