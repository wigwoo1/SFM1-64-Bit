"""Private complete double-bit comparison for additional parser grammar inputs."""
import json,random,struct,subprocess,tempfile
from verify_material_numeric_value import ROOT,PRIVATE,LIB,PIN,require,sha
def main():
    out=ROOT/'outputs/material-value-construction-20261025';run=type(out)(tempfile.mkdtemp(prefix='parser-verification-',dir=out))
    require(sha(PRIVATE)==sha(LIB)==PIN,'Library identity');cases=[]
    for prefix in ['', '+','-',' ','\t','\n','\v','\f','\r',' -']:
        for s in ['','0','00','.0','.','1.5','1e3','1d3','1D-3','1e','1e+','1e-','1+3','1-3','nan','inf','0x1p2','1.#INF','1\0e9','12suffix','-x']:
            cases.append((prefix+s).encode())
    for retained in range(10):
        for discarded in range(10):
            for suffix in ['','e-24','e-324','e300']:
                cases.append(('12345678901234567890123'+str(retained)+str(discarded)+suffix).encode())
    rng=random.Random(2026102505)
    for _ in range(5000):
        digits=''.join(str(rng.randrange(10)) for _ in range(rng.randrange(1,150)))
        at=rng.randrange(len(digits)+1);s=digits[:at]+'.'+digits[at:]
        s=rng.choice(['','-','+','\t'])+s+rng.choice(['e','E','d','D'])+str(rng.randrange(-9999,10000))
        cases.append(s.encode())
    query=run/'queries.bin';query.write_bytes(b''.join(struct.pack('<I',len(s))+s for s in cases))
    outputs=[];exes=[]
    for tag,suffix in [('native','x86'),('owned','x64')]:
        exe=out/f'material_construction_{tag}_{suffix}.exe';result=run/(tag+'.bin')
        p=subprocess.run([str(exe),'--parse-batch',str(PRIVATE),str(query),str(result)],capture_output=True,text=True,timeout=60)
        (run/(tag+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,'Parser execution '+tag)
        data=result.read_bytes();require(len(data)==8*len(cases),'Parser output size');outputs.append(data);exes.append(dict(path=str(exe),sha256=sha(exe)))
    diff=[dict(case=i,input_hex=s.hex(),native=outputs[0][8*i:8*i+8].hex(),owned=outputs[1][8*i:8*i+8].hex()) for i,s in enumerate(cases) if outputs[0][8*i:8*i+8]!=outputs[1][8*i:8*i+8]]
    (run/'results.json').write_text(json.dumps(dict(status='FAIL' if diff else 'PASS',cases=len(cases),differences=diff,executables=exes),indent=2)+'\n')
    print(json.dumps(dict(receipt=str(run/'results.json'),cases=len(cases),differences=len(diff),first=diff[:4])))
    require(not diff,'Parser bits parity')
if __name__=='__main__':main()
