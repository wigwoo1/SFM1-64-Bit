from pathlib import Path
import runpy,struct,sys,json,hashlib
recovered=runpy.run_path('work/audit_client_exp_helper.py');double=recovered['double'];f=recovered['f']
tables=b''.join(struct.pack('<d',double(base+index*8)) for base,count in [(0x105EF0C8,16),(0x105EEFC8,32),(0x105EEEC8,32),(0x105EF148,2)] for index in range(count))
values=[f(-0.0001-(9.9999*index/65535)) for index in range(65536)]
inputs=b''.join(struct.pack('<f',value) for value in values)
if '--prepare' in sys.argv:
 Path('work/client-exp-tables.bin').write_bytes(tables);Path('work/client-exp-input.bin').write_bytes(inputs);print('Prepared 65536 exponential cases');sys.exit()
assert Path('work/client-exp-tables.bin').read_bytes()==tables
assert Path('work/client-exp-input.bin').read_bytes()==inputs
expected=b''.join(struct.pack('<f',recovered['candidate'](value)) for value in values)
actual=Path('outputs/sfm64-client-exp-normal-output.bin').read_bytes();assert actual==expected
report=dict(cases=len(values),bit_exact=True,table_sha256=hashlib.sha256(tables).hexdigest(),input_sha256=hashlib.sha256(inputs).hexdigest(),output_sha256=hashlib.sha256(actual).hexdigest(),scope='x64 strict candidate versus independent recovered-path emulator, negative normal range only; not a live native oracle or complete exponential implementation')
Path('outputs/sfm64-client-exp-normal-validation.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print('All 65536 exponential candidate cases match emulator bits')
