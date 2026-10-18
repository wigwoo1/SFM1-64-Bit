from pathlib import Path
import hashlib,json,runpy
context=runpy.run_path('work/recover_constant_subobject_methods.py')
image=context['image'];verify=context['verified_span']
assert image.word(0x1016e20c)==0x10184048
assert image.string(0x1016e29c)=='.?AVCShaderManager@@'
assert image.word(0x100ff9b0)==0x1016adfc
assert image.word(0x1016adfc+12)==0x1016e294
assert image.word(0x100ff9b4+11*4)==0x10033900
startup=verify(0x100f9920,0x100f992a)
constructor=verify(0x10036760,0x1003676c)
assert bytes.fromhex(startup[0]['bytes'])==bytes.fromhex('b948401810')
assert bytes.fromhex(constructor[-1]['bytes'])==bytes.fromhex('c706b4f90f10')
rows=verify(0x10033900,0x1003392d)
raw=image.b[image.offset(0x10033900):image.offset(0x10033900)+45]
Path('work/shader_manager_restore_reset_native_init.h').write_text('#pragma once\nstatic const unsigned char managerRestoreResetBytes[]={'+','.join(map(str,raw))+'};\nstatic const unsigned managerRestoreResetFileOffset='+str(image.offset(0x10033900))+';\n')
report=dict(dll_sha256=context['expected'],global_pointer='0x1016e20c',filebacked_initial_object='0x10184048',startup=startup,constructor_prefix=constructor,manager_table='0x100ff9b4',slot11='0x10033900',instructions=rows,downstream={str(slot):hex(image.word(0x100ff9b4+slot*4)) for slot in [9,10]},flow='Capture manager receiver; load its slot9 target; set fields9c/a0 toffffffff before callback9(ffffffff). Reload receiver vtable and invoke slot10(ffffffff). No field re-clear after callback9; receiver remains captured.',limits='Static installed bytes and startup/constructor/table provenance only. Downstream methods9/10 semantics and original runtime object changes remain unverified.')
Path('outputs/sfm64-shader-manager-restore-reset.json').write_text(json.dumps(report,indent=2)+'\n')
print('shader_manager_restore_reset_bytes=45 startup=10 constructor=12 verified')
