from pathlib import Path
import struct,math,json
b=Path('outputs/sfm64-flex-quantization.bin').read_bytes();assert len(b)==4+65536*4
scale=struct.unpack_from('<f',b)[0];accepted=0
for word in range(65536):
    value=struct.unpack('<e',struct.pack('<H',word))[0];expected=b'\xff'*4
    if math.isfinite(value):
        divided=struct.unpack('<f',struct.pack('<f',value/scale))[0];quantized=math.trunc(divided)
        if -32768<=quantized<=32767:expected=struct.pack('<f',quantized*scale);accepted+=1
    assert b[4+word*4:8+word*4]==expected,hex(word)
Path('outputs/sfm64-flex-quantization-validation.json').write_text(json.dumps(dict(scale=scale,encodings_checked=65536,accepted_bit_exact=accepted,rejected=65536-accepted,scope='SDK arithmetic candidate, native conversion invocation and live deformation unverified'),indent=2)+'\n',encoding='utf-8')
print(f'All65536 quantization encodings checked, accepted={accepted}, rejected={65536-accepted}')
