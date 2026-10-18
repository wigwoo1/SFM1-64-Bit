import hashlib,json,re
from pathlib import Path
from pe_image import Image
binary=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
image=Image(binary)
ranges={'bias_setter':(0x1001f360,0x1001f3bc),'bias_hardware_selection':(0x10016410,0x100164cc),'render_state_submission':(0x1000e330,0x1000e3fc)}
spans={name:[] for name in ranges};pending=None
for line in Path('work/shaderapidx9-disassembly.txt').open():
 match=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
 if match:
  if pending:
   address,data,name=pending;at=image.offset(address);assert image.b[at:at+len(data)]==data,(name,hex(address));spans[name].append(dict(address=hex(address),bytes=data.hex()));pending=None
  address=int(match[1],16)
  for name,(begin,end) in ranges.items():
   if begin<=address<end:pending=(address,bytes.fromhex(match[2]),name);break
 elif pending:
  continuation=re.fullmatch(r'\s+((?:[0-9A-F]{2}\s*)+)',line)
  if continuation:address,data,name=pending;pending=(address,data+bytes.fromhex(continuation[1]),name)
 if match and int(match[1],16)>=max(end for begin,end in ranges.values()) and not pending:break
for name,(begin,end) in ranges.items():
 cursor=begin
 for span in spans[name]:assert int(span['address'],16)==cursor;cursor+=len(bytes.fromhex(span['bytes']))
 assert cursor==end,(name,hex(cursor),hex(end))
# Exact instructions proving float-value storage, branch2 selection and D3D9 state arguments.
expected={0x1001f368:'f30f1181242c0000',0x1001f375:'f30f1181282c0000',0x10016461:'f30f1086242c0000',0x1001646e:'f30f1086282c0000',0x1001649a:'68af000000',0x100164b5:'68c3000000',0x1000e3de:'8b88e4000000'}
for address,raw in expected.items():data=bytes.fromhex(raw);at=image.offset(address);assert image.b[at:at+len(data)]==data
report=dict(binary=str(binary),binary_sha256=hashlib.sha256(image.b).hexdigest(),verified_instruction_spans=spans,
 bias_setter=dict(address='0x1001f360',argument1_storage='0x2c24',argument2_storage='0x2c28',dirty_byte='0x2e00'),
 hardware_branch2=dict(selector='snapshot byte0x44 bits2..3 == 2',slope_storage='0x2c24',depth_storage='0x2c28',slope_render_state=175,depth_render_state=195,submission_helper='0x1000e330',value_operation='float bit patterns passed without scaling in this branch'),
 direct_device_path=dict(device_vtable_offset='0xe4',state_cache=True,queued_submission_path=True),
 public_interface_source='https://raw.githubusercontent.com/ValveSoftware/source-sdk-2013/master/src/public/shaderapi/ishaderapi.h',
 limitations='Offline byte-verified subset. Setter identity/order inference cross-checked against public interface. End-to-end saved-light-to-setter dispatch and original hardware execution not yet proven. Vulkan mapping not enabled.')
Path('outputs/shadow-depth-bias-consumer-recovery.json').write_text(json.dumps(report,indent=2));print('Verified installed renderer bias spans: '+str(sum(map(len,spans.values()))))
