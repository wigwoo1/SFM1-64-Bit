from pathlib import Path
import struct,math,random,sys,json
f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
rng=random.Random(1937)
vectors=[(0.,0.,0.),(3.,4.,0.),(1.,0.,0.)]+[tuple(f32(rng.uniform(-8,8)) for _ in range(3)) for _ in range(4096)]
if '--prepare' in sys.argv:
 Path('work/software-flex-normalize-input.bin').write_bytes(b''.join(struct.pack('<3f',*v) for v in vectors))
else:
 data=Path('outputs/software-flex-normalize-result.bin').read_bytes();assert len(data)==len(vectors)*16
 for i,v in enumerate(vectors):
  squared=f32(f32(v[0]*v[0])+f32(v[1]*v[1]));squared=f32(squared+f32(v[2]*v[2]));length=f32(math.sqrt(squared));inverse=f32(1/f32(length+2**-23))
  expected=struct.pack('<4f',*(f32(x*inverse) for x in v),length)
  assert data[i*16:(i+1)*16]==expected,i
 Path('outputs/software-flex-normalize-validation.json').write_text(json.dumps(dict(fixtures=len(vectors),float32_bit_exact=True,zero_preserved=True,scope='Independent arithmetic reference for recovered scalar normalization order; no native execution oracle'),indent=2)+'\n',encoding='utf-8')
 print(f'{len(vectors)} normalization fixtures match independent float32 reference exactly')
