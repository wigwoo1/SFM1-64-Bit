from pathlib import Path
import struct
p=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\sdktools\python\2.7\win32\Lib\site-packages\vs\datamodel_pyd.pyd');data=p.read_bytes();pe=struct.unpack_from('<I',data,60)[0];base=struct.unpack_from('<I',data,pe+52)[0];table=pe+24+struct.unpack_from('<H',data,pe+20)[0]
for n in range(struct.unpack_from('<H',data,pe+6)[0]):
 o=table+n*40;size,rva,rawsize,raw=struct.unpack_from('<IIII',data,o+8)
 if rva<=0x10077616-base<rva+rawsize:
  start=raw+0x10077616-base-rva;kernel=data[start:start+0x10077692-0x10077616]+b'\xc3';break
else:raise RuntimeError('kernel missing')
if kernel[:4]!=bytes.fromhex('F3 0F 10 0E') or kernel[-6:-1]!=bytes.fromhex('F3 0F 11 4E 28'):
 raise RuntimeError('Current binary does not match the recorded scale disassembly; refresh disassembly before executing')
Path('work/native_local_scale_kernel.inc').write_text(','.join(f'0x{x:02x}' for x in kernel))
print('Extracted native SSE arithmetic bytes:',len(kernel))
