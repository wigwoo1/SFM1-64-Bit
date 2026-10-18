from pathlib import Path
from pe_image import Image
import re,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10243220,0x102434BF),(0x10241F50,0x10242428),(0x10242760,0x102427BF),(0x10242490,0x102424AC),(0x10242570,0x10242579),(0x1025EA80,0x1025EA97),(0x1025EB70,0x1025EB87)]
selected=[];pending=None
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
   if pending[0]>=max(end for begin,end in ranges):break
  elif pending:
   tail=re.fullmatch(r'\s+((?:[0-9A-F]{2} ?)+)\s*',line)
   if tail:pending[1]+=bytes.fromhex(tail[1])
assert image.b[image.offset(0x102433B0):image.offset(0x102433B0)+3]==bytes.fromhex('89 5E 1C')
assert image.b[image.offset(0x1024323A):image.offset(0x1024323A)+7]==bytes.fromhex('C7 46 04 04 B9 AD 10')
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),base_history_offset='0x1c',renderable_history_offset='0x18',constructor='0x10243220',constructor_null_store='0x102433b0',reset_release='0x102421c1',reset_allocation='0x10242235',reset_pointer_store='0x10242240',zero_loop='0x10242255',descriptor_count_offset='0x104',destruction_release='0x10242574',allocation_helper='0x1025ea80',release_helper='0x1025eb70',allocator_indirection='0x10a7e484',allocation_virtual_offset=4,release_virtual_offset=20,verified_complete_instructions=selected,limitations='Static selected lifecycle regions checked against file bytes. Setup caller identity, all conditional bypass branches, allocation failures, complete ownership/aliasing and native runtime scheduling remain unverified.')
assert image.string(0x10ADB8E8)=='CDmePuppet::OnNewModel'
assert image.string(0x10ADB9F0)=='CDmePuppet::UpdateModel'
report['setup_routine']='0x10241f50'
report['setup_profiler_name']=image.string(0x10ADB8E8)
report['update_routine']='0x10242760'
report['update_profiler_name']=image.string(0x10ADB9F0)
report['observed_direct_callers']=['0x102427ad','0x10243499']
report['caller_gate']='Both observed calls follow inequality between resolved model EAX and stored model pointer at puppet base+0x34. UpdateModel exits for an empty model name at base+0x30.'
report['limitations']='Complete setup/update/constructor instructions and selected destruction/helpers checked against file bytes. Direct caller search is disassembly-based, not proof of absence of indirect dispatch. Model resolution, all bypass branch semantics, allocation failures, runtime scheduling and complete ownership/aliasing remain unverified.'
Path('outputs/sfm64-ifm-puppet-history-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified history lifecycle instructions:',len(selected))
