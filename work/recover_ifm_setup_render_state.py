from pathlib import Path
from pe_image import Image
import re,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');selected=[];pending=None
ranges=[(0x102AE740,0x102AEB04),(0x102AE0A0,0x102AE0C7),(0x102AEDAD,0x102AEDD2),(0x102AEE5B,0x102AEE83)]
def save(item):
 if item is None:return
 address,data,line=item
 if not any(begin<=address<end for begin,end in ranges):return
 offset=image.offset(address);assert image.b[offset:offset+len(data)]==data
 selected.append(dict(address=hex(address),bytes=data.hex(),instruction=line))
with Path('work/ifm-disassembly.txt').open(encoding='utf-8') as stream:
 for line in stream:
  match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
  if match:
   save(pending);pending=[int(match[1],16),bytes.fromhex(match[2]),line.strip()]
   if pending[0]>=0x102AEED0:break
  elif pending:
   tail=re.fullmatch(r'\s+((?:[0-9A-F]{2} ?)+)\s*',line)
   if tail:pending[1]+=bytes.fromhex(tail[1])
names={hex(address):image.string(address) for address in (0x10AE6488,0x10AE6464,0x10AE6444)}
assert names['0x10ae6488']=='CMovieDoc::SetupRenderState'
assert names['0x10ae6464']=='g_pDmElementFramework->Operate()'
assert names['0x10ae6444']=='CMovieDoc::BuildOperatorList'
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),routine='0x102ae740',profiler_names=names,operator_list_routine='0x102ae0a0',operate_receiver_global='0x10e9ccf8',operate_virtual_offset='0x2c',operate_call='0x102ae7cd',caller_time_writes=['0x102aedc1','0x102aee72'],verified_complete_instructions=selected,limitations='Installed profiler operands support routine names. Complete SetupRenderState instructions and selected caller/build-list bytes verified. Operate interface binding/implementation, downstream helpers, live operator evaluation and draw scheduling remain unverified. SetupRenderState is preparation, not evidence of a completed renderer draw.')
Path('outputs/sfm64-ifm-setup-render-state-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified setup-render-state instructions:',len(selected),names)
