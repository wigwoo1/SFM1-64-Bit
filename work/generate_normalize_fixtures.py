import pathlib,random
r=random.Random(6031);rows=[]
for _ in range(2000):
 value=r.getrandbits(96);exponent=r.choice([-100,-20,-1,0,1,20,100,16383,32766]);initial=exponent
 if exponent>0:
  shift=min(exponent,96-value.bit_length());value<<=shift;exponent-=shift
 if exponent<=0:
  shift=1-exponent;lost=value&((1<<shift)-1);value>>=shift
  if lost:value|=1
  exponent=0
 guard=value&65535;retained=value>>16
 if guard>32768 or (guard==32768 and retained&1):retained+=1
 if retained>=1<<80:retained=1<<79;exponent+=1
 if exponent>=32767:retained=1<<79;exponent=32767
 # Regenerate input independently of transformed value using saved PRNG data below.
 # Input is captured separately for reproducible C++ fixture generation.
 rows.append((initial,exponent,retained))
# Repeat seed to obtain original inputs; consumes the same random choices.
r=random.Random(6031);render=[]
for initial,exponent,retained in rows:
 original=r.getrandbits(96);assert r.choice([-100,-20,-1,0,1,20,100,16383,32766])==initial
 limbs=[(original>>(16*i))&65535 for i in range(6)]
 expected=[(retained>>(16*i))&65535 for i in range(5)]
 render.append('{{'+','.join(map(str,limbs))+'},'+str(initial)+',{{'+','.join(map(str,expected))+'},'+str(exponent)+'}}')
pathlib.Path('work/extended_normalize_fixtures.inc').write_text(',\n'.join(render))
print('Generated',len(render),'arbitrary-precision normalization fixtures')
