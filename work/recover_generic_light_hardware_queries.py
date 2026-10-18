"""Resolve file-backed hardware object and byte-check light upload queries."""
import hashlib,json
from pathlib import Path
from pe_image import Image
image=Image(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll'))
pointer=image.word(0x1016d130);assert pointer==0x1016d150
table=image.word(pointer);assert table==0x100fa6b4
assert image.word(table+3*4)==0x10003220
assert image.word(table+12*4)==0x100034b0
spans=[]
for address,expected in [(0x10003220,'8a81ac0500002401c3'),(0x100034b0,'8b8144050000c3')]:
 data=bytes.fromhex(expected);at=image.offset(address);assert image.b[at:at+len(data)]==data
 spans.append({'address':hex(address),'bytes':expected})
report={
 'binary_sha256':hashlib.sha256(image.b).hexdigest(),
 'global_pointer':hex(pointer),'vtable':hex(table),
 'slot12':'100034b0 returns raw32-bit word at hardware object+0x544. Upload comparisons interpret count as signed. This is the same config field used by retained light-array setter10010bb0 before its min(config,4,requested).',
 'slot3':'10003220 returns low byte of hardware object+0x5ac masked with1. Other return-register bytes are not defined; callers inspect AL.',
 'file_initial_count':image.word(pointer+0x544),
 'file_initial_skip_flag':image.b[image.offset(pointer+0x5ac)]&1,
 'consequence':'Repeated count queries in unused-light path are simple reads of the same field for this file-backed vtable. This does not prove absence of runtime mutation or pointer replacement.',
 'verified_code_spans':spans,
 'limits':'Runtime configuration population, logical default-light data extent, original object initialization and complete upload execution/GPU behavior remain unverified. File initial count is not a proven supported runtime limit.'}
Path('outputs/sfm64-generic-light-hardware-queries-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('light_hardware_queries=2 file_backed_pointer_and_vtable=verified shared_config_count_offset=0x544')
