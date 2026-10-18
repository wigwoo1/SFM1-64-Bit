import pathlib,random
r=random.Random(1847)
pairs=[([0]*5,[0]*5),([65535]*5,[65535]*5)]
for i in range(5):
 for j in range(5):
  a=[0]*5;b=[0]*5;a[i]=65535;b[j]=65535;pairs.append((a,b))
for _ in range(2000):pairs.append(([r.randrange(65536) for _ in range(5)],[r.randrange(65536) for _ in range(5)]))
rows=[]
for a,b in pairs:
 v=sum(a[i]*b[j]*(1<<(16*(i+j-4))) for i in range(5) for j in range(5) if i+j>=4)
 expected=[(v>>(16*k))&65535 for k in range(6)]
 rows.append('{ {'+','.join(map(str,a))+'}, {'+','.join(map(str,b))+'}, {'+','.join(map(str,expected))+'} }')
pathlib.Path('work/extended_product_fixtures.inc').write_text(',\n'.join(rows))
print('Generated',len(rows),'arbitrary-precision polynomial fixtures')
