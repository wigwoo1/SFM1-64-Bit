"""Byte-checked per-light upload routing, including integer/boolean metadata."""
import hashlib,json,re
from pathlib import Path
from pe_image import Image
image=Image(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll'))
lines=Path('work/shaderapidx9-disassembly.txt').read_text().splitlines();spans=[]
for begin,end in [(0x10013040,0x1001336f),(0x100100a0,0x100101cd),(0x100101e0,0x10010325)]:
 cursor=begin
 for n,line in enumerate(lines):
  m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
  if not m or not begin<=int(m[1],16)<end:continue
  address=int(m[1],16);data=bytes.fromhex(m[2])
  extra=re.fullmatch(r'\s+((?:[0-9A-F]{2}\s*)+)',lines[n+1])
  if extra:data+=bytes.fromhex(extra[1])
  assert address==cursor;offset=image.offset(address)
  assert image.b[offset:offset+len(data)]==data
  spans.append({'address':hex(address),'bytes':data.hex()});cursor+=len(data)
 assert cursor==end,(hex(cursor),hex(end))
targets={str(slot):hex(image.word(0x100fcedc+slot*4)) for slot in (41,42)}
report={
 'binary_sha256':hashlib.sha256(image.b).hexdigest(),
 'routine':'10013040, one record argument, adjusted state base',
 'active_float_upload':'Signed record count at byte0x60. Positive count compares count*80 bytes from record+0xc4 with retained cache+0x1b0. Difference uploads count*5 vertex float4 constants starting register27; unchanged skips.',
 'device_or_queue':'Direct device vtable byte0x178 (slot94) or queued command10 containing register,count and copied float words. Successful path copies same bytes into caches referenced at adjusted base+0x19e0 and+0x2858.',
 'unused_slot_policy':'Query global1016d130 interface slot12. If record count is smaller, query slot3; low result byte nonzero skips fill. Otherwise query slot12 twice more to compute missing*5 constant count and starting register27+5*count. Source block is global1016dd10. Compare and upload/copy only on mismatch.',
 'unused_slot_limits':'Runtime source-block contents/initialization and semantic names of hardware queries remain unresolved. Repeated queries must not silently collapse without proving stability. Negative count behavior is not bounded by this original routine.',
 'metadata_calls':[
  {'interface_slot':42,'target':targets['42'],'register':0,'count':1,'source_record_byte':'0x204','fourth_argument':0,'kind':'vertex integer constants: count*16 bytes, device vtable byte0x180/slot96'},
  {'interface_slot':41,'target':targets['41'],'register':0,'count':4,'source_record_byte':'0x214','fourth_argument':0,'kind':'vertex boolean constants: count*4 bytes, device vtable byte0x188/slot98'}],
 'metadata_cache_policy':'Fourth argument low byte nonzero forces upload; otherwise compare complete range bitwise and skip identical. Each successful upload copies values into two retained caches. Queue branches present; queue command layout not yet incorporated into owned planner.',
 'integration_limit':'Existing owned flat float-upload plan cannot represent these metadata calls. Do not claim full native light upload integration from float ranges alone.',
 'verified_instruction_spans':spans,
 'limits':'Offline machine-byte recovery only. No original DLL initialization, runtime source initialization, complete routine execution oracle or GPU parity claimed.'}
Path('outputs/sfm64-generic-light-upload-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('light_upload_instruction_spans='+str(len(spans)))
print('metadata_targets='+str(targets))
