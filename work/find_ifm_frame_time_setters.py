from pathlib import Path
from pe_image import Image
import re,json,hashlib,struct
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
windows=[];active=[]
with Path('work/ifm-disassembly.txt').open(encoding='utf-8') as stream:
 for line in stream:
  for window in active:window.append(line.rstrip())
  if '[10FD4398h]' in line:active.append([line.rstrip()])
  done=[window for window in active if len(window)>=24]
  for window in done:
   active.remove(window)
   if any(re.search(r'\[(?:eax|edx)\+000000C8h\]',item) for item in window):windows.append(window)
starts=[int(re.match(r'\s+([0-9A-F]{8}):',window[0])[1],16) for window in windows if any('000000C8h' in item for item in window[:5])]
ranges=[(0x102AED8D,0x102AEDCD),(0x102AEE50,0x102AEE83),(0x102AF27A,0x102AF2DA),(0x102B271A,0x102B2754),(0x102B78AD,0x102B7904),(0x102B79C0,0x102B7A0D),(0x102B8034,0x102B8058)]
checked=[];pending=None
def save(item):
 if item is None:return
 address,data,line=item
 if not any(begin<=address<end for begin,end in ranges):return
 offset=image.offset(address);assert image.b[offset:offset+len(data)]==data
 checked.append(dict(address=hex(address),bytes=data.hex(),instruction=line))
with Path('work/ifm-disassembly.txt').open(encoding='utf-8') as stream:
 for line in stream:
  match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
  if match:
   save(pending);pending=[int(match[1],16),bytes.fromhex(match[2]),line.strip()]
   if pending[0]>=0x102B8060:break
  elif pending:
   tail=re.fullmatch(r'\s+((?:[0-9A-F]{2} ?)+)\s*',line)
   if tail:pending[1]+=bytes.fromhex(tail[1])
constant=struct.unpack_from('<f',image.b,image.offset(0x10A8DFD4))[0]
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),windows=windows,short_receiver_chain_candidates=[hex(v) for v in starts],verified_complete_instructions=checked,tick_scale_address='0x10a8dfd4',tick_scale=constant,limitations='Proximity search with seven short receiver-chain candidates; longer-lived/indirect calls may be missed. Selected argument/dispatch bytes checked, but surrounding scheduling and helper semantics remain unverified. The 1027D920 window uses another interface for its C8 call and is excluded from short-chain candidates.')
Path('outputs/sfm64-ifm-frame-time-setter-candidates.json').write_text(json.dumps(report,indent=2)+'\n')
print('Proximity windows:',len(windows),'short-chain candidates:',len(starts),'verified instructions:',len(checked),'tick scale:',constant)
