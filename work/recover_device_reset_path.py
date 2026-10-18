from pathlib import Path
import hashlib,json,re
from pe_image import Image
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
ranges=[(0x1002b660,0x1002b6ff),(0x1002b8c1,0x1002b8cc),(0x1002cc00,0x1002cc04),(0x10029960,0x10029cf1)];rows=[];pending=None
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 match=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
 if match:
  pending=None;address=int(match[1],16)
  if any(start<=address<end for start,end in ranges):pending={'address':hex(address),'bytes':bytes.fromhex(match[2]).hex(),'instruction':match[3].strip()};rows.append(pending)
 elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):pending['bytes']+=bytes.fromhex(line).hex()
covered=set()
for row in rows:
 address=int(row['address'],16);raw=bytes.fromhex(row['bytes']);offset=image.offset(address);assert image.b[offset:offset+len(raw)]==raw;covered.update(range(address,address+len(raw)))
assert covered=={address for start,end in ranges for address in range(start,end)}
report={'dll_sha256':hashlib.sha256(image.b).hexdigest(),'verified_bytes':len(covered),'regions':[{'start':hex(start),'end':hex(end),'bytes':end-start} for start,end in ranges],'instructions':rows,
 'owner_descriptor':'1002cc00 returns owner+70;1002b8c1 supplies it to1002b660. Descriptor semantic fields/pointer widths still unresolved; do not equate its56-byte size with D3DPRESENT_PARAMETERS.',
 'reset_flow':['Clear owner byteac bit3 before descriptor flag+34 bit1 gate; set gate returnsfalse.', 'Call10029960 with owner+8,owner+24,descriptor,true; manager10028a80; owner vtable slot0(true); call10029960 again with same input,false.', 'Getter1000df50 returns command state; call10002e50 flush BEFORE loading command-state device pointer. Device COMslot16 called with owner+30 presentation pointer.', 'Negative HRESULT logs then returnsfalse. Nonnegative calls1002b4c0(true,true,string100fe938) then returnstrue.'],
 'event_cache_write_in_selected_reset_body':False,'queue_consumer_identified':False,'native_executed':False,'presentation_builder':{'first_pass':'Nonzero final argument skips all owner+30 presentation writes after display/aspect callbacks.','second_pass':'Clears56-byte PE32 presentation block; descriptor flag bit0 selects Windowed; owner+4c receives incoming window handle.','fields':{'owner+30':'BackBufferWidth','owner+34':'BackBufferHeight','owner+38':'BackBufferFormat from10036f30','owner+3c':'BackBufferCount','owner+40':'MultiSampleType','owner+44':'MultiSampleQuality','owner+48':'SwapEffect=(descriptor byte34 OR40)>>6','owner+4c':'hDeviceWindow, pointer requiring widening','owner+50':'Windowed=descriptor byte34 AND1','owner+54':'EnableAutoDepthStencil=1','owner+58':'AutoDepthStencilFormat from100026f0','owner+5c':'Flags remains zero after block clear','owner+60':'FullScreen_RefreshRateInHz signed ratio descriptor14/18 or0','owner+64':'PresentationInterval'},'multisample':'Device API COMslot11 with adapter, device type1, format, Windowed, sample type and quality output; includes vendor10de/request16 special handling.','limits':'Field mapping is static interpretation of stores and documented layout; dependent builders, display callbacks, format mapping and real GPU execution remain unverified.'},'limits':'Selected reset helper and descriptor forwarding verified statically. Presentation-builder runtime/dependency binding, manager/base hooks, cache invalidation across other lifecycle routines, real device reset and queue consumption unresolved.'}
Path('outputs/sfm64-device-reset-path-recovery.json').write_text(json.dumps(report,indent=2)+'\n');print('device_reset_path_verified_bytes='+str(len(covered)))


