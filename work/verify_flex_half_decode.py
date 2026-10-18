from pathlib import Path
import math,struct,json
actual=Path('outputs/sfm64-flex-half-decoded.bin').read_bytes()
assert len(actual)==65536*4
finite=0
for word in range(65536):
    value=struct.unpack('<e',struct.pack('<H',word))[0]
    expected=struct.pack('<f',value) if math.isfinite(value) else b'\xff'*4
    assert actual[word*4:word*4+4]==expected,hex(word)
    finite+=math.isfinite(value)
Path('outputs/sfm64-flex-half-decode-validation.json').write_text(json.dumps(dict(encodings=65536,finite_bit_exact=finite,nonfinite_rejected=65536-finite,reference='Python struct IEEE half to float, including signed zero and subnormals',scope='Half decoder only; native SFM encoding selection and deformation unverified'),indent=2)+'\n',encoding='utf-8')
print(f'all 65536 half encodings checked: {finite} finite bit-exact, {65536-finite} nonfinite rejected')
