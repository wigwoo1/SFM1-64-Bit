"""Private exact-integer reconstruction of the decimal scaling stage."""
import json,random,re,struct,subprocess,tempfile
from datetime import datetime,timezone
from verify_material_decimal_powers import power80
from verify_ifm_draw_packet import check_ranges
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha

def multiply(a,ea,b,eb):
    total=ea+eb
    if ea>=32767 or eb>=32767 or total>0xbffd:return 1<<79,32767
    if total<=0x3fbf:return 0,0
    if ea==0:
        total+=1
        if a==0:return 0,0
    if eb==0:
        total+=1
        if b==0:return 0,0
    # Only diagonals >=4 participate. Discarded diagonals supply no carry.
    p=sum(((a>>(16*i))&65535)*((b>>(16*j))&65535)<<(16*(i+j-4))
          for i in range(5) for j in range(5) if i+j>=4)
    e=total-16382
    while e>0 and p<(1<<95):p<<=1;e-=1
    if e<=0:
        e-=1
        if e<0:
            shift=-e;lost=p&((1<<shift)-1);p>>=shift
            if lost:p|=1
            e=0
    low=p&65535;m=p>>16
    if low>32768 or (low==32768 and m&1):m+=1
    if m==(1<<80):m=1<<79;e+=1
    if e>=32767:return 1<<79,32767
    return m,e

def scale(n,k):
    while n%10==0:n//=10;k+=1
    if k>5200:return 1<<79,32767
    if k< -5200:return 0,0
    e=n.bit_length()-1;m=n<<(79-e);e+=16383
    if not k:return m,e
    m &= ~65535
    sign=1 if k>0 else -1;k=abs(k);group=0
    while k:
        digit=k&7;k>>=3
        if digit:
            b,eb=power80(sign*digit*(8**group));m,e=multiply(m,e,b,eb)
        group+=1
    return m,e

def main():
    out=ROOT/'outputs/material-value-construction-20261025'
    run=type(out)(tempfile.mkdtemp(prefix='scaling-verification-',dir=out))
    require(sha(LIB)==sha(PRIVATE)==PIN,'Library identity')
    _,evidence=check_ranges(LIB,'materialsystem-disassembly.txt',[
        ('scaling_selection_multiply_normalize_round',0x10087ece,0x100881c9)])
    rng=random.Random(2026102503)
    cases=[(1,k) for k in range(-5220,5221)]
    cases += [(rng.randrange(1,10**rng.randrange(1,25)),rng.randrange(-5220,5221)) for _ in range(5000)]
    cases += [(10**j,k) for j in range(24) for k in [-5201,-4932,-324,-1,0,1,308,4932,5201]]
    query=run/'queries.txt';query.write_bytes(('\n'.join(f'{n}e{k}' for n,k in cases)+'\n').encode())
    exe=out/'material_construction_native_x86.exe';result=run/'native.bin'
    p=subprocess.run([str(exe),'--wide-batch',str(PRIVATE),str(query),str(result)],capture_output=True,text=True,timeout=60)
    (run/'native.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Native parser batch')
    data=result.read_bytes();require(len(data)==12*len(cases),'Native frame count')
    differences=[];owned=bytearray()
    for i,(n,k) in enumerate(cases):
        m,e=scale(n,k);expected=m.to_bytes(10,'little')+e.to_bytes(2,'little');owned.extend(expected)
        if expected!=data[12*i:12*i+12]:differences.append(dict(case=i,n=str(n),k=k,native=data[12*i:12*i+12].hex(),model=expected.hex()))
    (run/'model.bin').write_bytes(owned)
    owned_exe=out/'material_construction_owned_x64.exe';owned_result=run/'owned.bin'
    p=subprocess.run([str(owned_exe),'--wide-batch',str(PRIVATE),str(query),str(owned_result)],capture_output=True,text=True,timeout=60)
    (run/'owned.log').write_text(p.stdout+p.stderr)
    require(p.returncode==0,'Owned scaling batch')
    require(owned_result.read_bytes()==owned,'Owned x64 versus integer scaling model')
    for tag,path,machine in [('native',exe,0x14c),('owned',owned_exe,0x8664)]:
        raw=path.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        require(struct.unpack_from('<H',raw,pe+4)[0]==machine,'PE architecture')
        require(not re.search(r'\b(warning|error)\b',(out/(tag+'-build.log')).read_text(),re.I),'Clean component build')
    require(sha(LIB)==sha(PRIVATE)==PIN,'Libraries unchanged')
    receipt=dict(status='PASS' if not differences else 'FAIL',cases=len(cases),differences=differences,time_utc=datetime.now(timezone.utc).isoformat(),
        executable=str(exe),executable_sha256=sha(exe),library_sha256=PIN,query_sha256=sha(query),native_sha256=sha(result),model_sha256=sha(run/'model.bin'),
        owned_executable=str(owned_exe),owned_executable_sha256=sha(owned_exe),owned_output_sha256=sha(owned_result),
        static_evidence=evidence,scope='Positive nonzero 1..24-digit integer with decimal exponent; original x86, owned x64, Python integer model')
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(receipt=str(run/'results.json'),cases=len(cases),differences=len(differences),first=differences[:3])))
    require(not differences,'Scaling model parity')
if __name__=='__main__':main()
