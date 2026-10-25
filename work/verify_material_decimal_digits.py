"""Compare decimal digit accumulation with original helper and exact integers."""
import json,random,re,struct,subprocess,tempfile
from datetime import datetime,timezone
from verify_ifm_draw_packet import check_ranges
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha

def main():
    out=ROOT/'outputs/material-value-construction-20261025'
    run=type(out)(tempfile.mkdtemp(prefix='digits-verification-',dir=out))
    require(sha(LIB)==sha(PRIVATE)==PIN,'Library identity')
    _,evidence=check_ranges(LIB,'materialsystem-disassembly.txt',[
        ('complete_digit_integer_and_normalization',0x1008f65e,0x1008f842)])
    rng=random.Random(2026102502);cases=[]
    for length in range(1,25):
        cases+=['1'+'0'*(length-1),'9'*length,'0'*(length-1)+'1']
        cases += [str(rng.randrange(1,10**length)).zfill(length) for _ in range(250)]
    for bit in range(80):
        for delta in [-1,0,1]:
            n=(1<<bit)+delta
            if 0<n<10**24:cases.append(str(n))
    query=run/'digits.txt';query.write_bytes(('\n'.join(cases)+'\n').encode())
    oracle=bytearray()
    for s in cases:
        n=int(s);exponent=n.bit_length()-1
        oracle.extend((n<<(79-exponent)).to_bytes(10,'little'))
        oracle.extend(struct.pack('<H',16383+exponent))
    exes=[]
    for name,machine,suffix in [('native',0x14c,'x86'),('owned',0x8664,'x64')]:
        exe=out/f'material_construction_{name}_{suffix}.exe'
        raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        require(struct.unpack_from('<H',raw,pe+4)[0]==machine,'PE machine')
        log=out/(name+'-build.log');require(not re.search(r'\b(warning|error)\b',log.read_text(),re.I),'Clean build')
        result=run/(name+'.bin')
        p=subprocess.run([str(exe),'--digits-batch',str(PRIVATE),str(query),str(result)],capture_output=True,text=True,timeout=30)
        (run/(name+'.log')).write_text(p.stdout+p.stderr)
        require(p.returncode==0,'Helper execution')
        require(result.read_bytes()==oracle,'Exact normalized integer bytes: '+name)
        exes.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),output_sha256=sha(result),build_log_sha256=sha(log)))
    require(sha(LIB)==sha(PRIVATE)==PIN,'Libraries unchanged')
    require(sha(ROOT/'work/material_string_value_candidate.h')=='4a9b16d46bd01e16f034922777e94cfb0d700ea614ab16198ec46c0bf8256007','Protected setter unchanged')
    receipt=dict(status='VERIFIED digit accumulation only',time_utc=datetime.now(timezone.utc).isoformat(),cases=len(cases),exact_bytes=len(oracle),
        domain='Nonzero integer represented by 1..24 decimal digits, including leading zeros',query_sha256=sha(query),executables=exes,
        source_sha256=sha(ROOT/'work/material_decimal_digits_candidate.h'),static_evidence=evidence,
        limitations=['Decimal scaling and final rounding not covered','Constructor acceptance still failed','Not integrated into editor'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(receipt=str(run/'results.json'),cases=len(cases),exact_bytes=len(oracle))))
if __name__=='__main__':main()
