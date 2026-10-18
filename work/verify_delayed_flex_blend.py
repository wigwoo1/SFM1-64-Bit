from pathlib import Path
import struct,random,json,sys,hashlib
def f(value):return struct.unpack('<f',struct.pack('<f',value))[0]
randomizer=random.Random(487220)
cases=[]
for index in range(65536):
    current=f(randomizer.uniform(-8,8));history=f(randomizer.uniform(-8,8))
    coefficient=f([0,1,0.8,0.5][index%4] if index<1024 else randomizer.random())
    cases.append((current,history,coefficient))
data=b''.join(struct.pack('<3f',*case) for case in cases)
if '--prepare' in sys.argv:
    Path('work/delayed-flex-blend-input.bin').write_bytes(data);print('Prepared',len(cases),'cases');sys.exit()
assert Path('work/delayed-flex-blend-input.bin').read_bytes()==data
expected=b''.join(struct.pack('<f',f(f(current*f(1-coefficient))+f(history*coefficient))) for current,history,coefficient in cases)
actual=Path('outputs/sfm64-delayed-flex-blend-output.bin').read_bytes()
assert actual==expected
report=dict(cases=len(cases),bit_exact=True,input_sha256=hashlib.sha256(data).hexdigest(),output_sha256=hashlib.sha256(actual).hexdigest(),scope='x64 strict candidate versus independent scalar float32 operation-order reference; no native runtime, coefficient timing, history initialization or seeking oracle')
Path('outputs/sfm64-delayed-flex-blend-validation.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print('All',len(cases),'blend cases match float32 reference exactly')
