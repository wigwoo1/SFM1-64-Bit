from pathlib import Path
import json,runpy,struct

# Reuse the installed-hash check and exact span validator, not inferred names.
context=runpy.run_path('work/recover_constant_subobject_methods.py')
image=context['image'];rows=context['verified_span'](0x100156a0,0x10015bc4)
assert image.word(0x100fcedc+154*4)==0x100156a0
starts={int(row['address'],16) for row in rows}
for row in rows:
 if row['instruction'].startswith('j'):
  target=int(row['instruction'].split()[1],16);assert target in starts
constants={hex(a):dict(bits=hex(image.word(a)),float32=struct.unpack('<f',struct.pack('<I',image.word(a)))[0]) for a in [0x100fa5f8,0x100fa5fc,0x100fc88c,0x100fc888,0x100fc884,0x100fc880,0x100fc87c,0x100fc878,0x100fc874]}
phases=[
 dict(register=0,compare='0x100156ef',queued_start='0x10015770',direct_call='0x100157c5',payload=['zero','constant100fa5f8','constant100fa5fc','constant100fc88c']),
 dict(register=1,compare='0x1001583e',queued_start='0x100158b2',direct_call='0x10015902',payload=['constant100fc888','input float32','constant100fc884','constant100fa5f8 / input float32 (DIVSS)']),
 dict(register=3,compare='0x1001597d',queued_start='0x100159f1',direct_call='0x10015a46',payload=['constant100fc880','constant100fc87c','constant100fc878','constant100fc874']),
 dict(register=225,compare='0x10015aa3',queued_start='0x10015b17',direct_call='0x10015b6f',payload=['zero','zero','zero','zero']),
]
report=dict(dll_sha256=context['expected'],target='0x100156a0',verified_bytes=0x10015bc4-0x100156a0,instructions=rows,constants=constants,phases=phases,policy='Each float4 compares primary cache (subobject+1710) bitwise; equal skips submission and cache publication. Changed writes opcode10/start/count1 or device slot94, then live primary and secondary (subobject+2588) caches. Input division uses SSE single precision. No clamp observed.',restore_argument_bits=hex(image.word(0x100fa5fc)),remaining_service='global1016e20c slot11, actual manager implementation not traced here',limits='Static installed-byte evidence; no full-method execution, FP exception/rounding parity, cookie binding, or actual GPU integration.')
Path('outputs/sfm64-constant-restore-tail.json').write_text(json.dumps(report,indent=2)+'\n')
print('constant_restore_tail_verified_bytes='+str(report['verified_bytes'])+' phases=4 input_bits='+report['restore_argument_bits'])
