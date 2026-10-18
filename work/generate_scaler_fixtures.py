import json,pathlib,random
table={int(e['address'],16):e for e in json.loads(pathlib.Path('outputs/sfm-decimal-scaling-tables.json').read_text())['entries']}
def decode(e):
 w=[int(v,16) for v in e['words']];return (w[0]|w[1]<<32|(w[2]&65535)<<64,w[2]>>16)
def multiply(left,right):
 a,ae=left;b,be=right;sign=(ae^be)&32768;ea=ae&32767;eb=be&32767;s=ea+eb
 if ea>=32767 or eb>=32767 or s>49149:return (1<<79,32767|sign)
 if s<=16319:return (0,0)
 if ea==0:
  s+=1
  if a==0:return (0,0)
 if eb==0:
  s+=1
  if b==0:return (0,0)
 v=sum(((a>>(16*i))&65535)*((b>>(16*j))&65535)*(1<<(16*(i+j-4))) for i in range(5) for j in range(5) if i+j>=4)
 exponent=s-16382
 if exponent>0:
  shift=min(exponent,96-v.bit_length());v<<=shift;exponent-=shift
 if exponent<=0:
  shift=1-exponent;lost=v&((1<<shift)-1);v>>=shift
  if lost:v|=1
  exponent=0
 guard=v&65535;retained=v>>16
 if guard>32768 or guard==32768 and retained&1:retained+=1
 if retained>=1<<80:retained=1<<79;exponent+=1
 if exponent>=32767:retained=1<<79;exponent=32767
 return retained,exponent|sign
def scale(value,exponent):
 magnitude=abs(exponent);group=0;base=0x10f229e8 if exponent<0 else 0x10f22888
 if magnitude:value=(value[0]&~65535,value[1])
 while magnitude:
  digit=magnitude&7;magnitude>>=3
  if digit:
   multiplier=decode(table[base+84*group+12*(digit-1)])
   if multiplier[0]&65535>=32768:
    m,e=multiplier;middle=((m>>16)&0xffffffff)-1;multiplier=(m&~(0xffffffff<<16)|(middle&0xffffffff)<<16,e)
   value=multiply(value,multiplier)
  group+=1
 return value
def render(value):return '{{'+','.join(str((value[0]>>(16*i))&65535) for i in range(5))+'},'+str(value[1])+'}'
cases=[((1<<79,16383),e) for e in range(-5200,5201)]
r=random.Random(415)
cases += [((r.getrandbits(79)|(1<<79),r.randrange(16000,17000)),r.randrange(-5200,5201)) for _ in range(500)]
rows=['{'+render(v)+','+str(e)+','+render(scale(v,e))+'}' for v,e in cases]
pathlib.Path('work/decimal_scaler_fixtures.inc').write_text(',\n'.join(rows))
print('Generated',len(rows),'integer-oracle scaler fixtures')
