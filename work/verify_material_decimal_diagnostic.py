"""Private diagnostic only: expose native intermediate, not owned acceptance."""
from pathlib import Path
import hashlib,json,math,re,struct,subprocess,tempfile
from datetime import datetime,timezone
from verify_ifm_draw_packet import check_ranges
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha

def main():
    out=ROOT/'outputs/material-value-construction-20261025'
    run=Path(tempfile.mkdtemp(prefix='decimal-diagnostic-',dir=out))
    exe=out/'material_construction_native_x86.exe'
    require(sha(LIB)==sha(PRIVATE)==PIN,'Pinned original/private library identity')
    _,evidence=check_ranges(LIB,'materialsystem-disassembly.txt',[
        ('parser_wrapper_arguments',0x100868e0,0x10086987),
        ('integer_digit_limit',0x10087ce9,0x10087d07),
        ('fraction_digit_limit',0x10087d43,0x10087d5f),
        ('reduce_25_digits_to_24',0x10087e03,0x10087e25),
        ('wide_converter_input',0x1008e861,0x1008e8ae)])
    for tag in ['native','owned']:
        require(not re.search(r'\b(warning|error)\b',(out/(tag+'-build.log')).read_text(),re.I),'Clean '+tag+' build')
    raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
    require(struct.unpack_from('<H',raw,pe+4)[0]==0x14c,'Reference is x86')
    failures=json.loads((out/'verification-cibel3w6/failure-results.json').read_text())['all_mismatches']
    texts=list(dict.fromkeys(bytes.fromhex(x['input_hex']).decode() for x in failures))
    base='999999999999999944488848768742172978818416595458984375'
    neighbors=[base+str(x)+'e-55' for x in [0,1,2]]
    texts=list(dict.fromkeys(texts+neighbors+['6459691196903481.5','1e3','16777217']))
    results=[]
    for text in texts:
        def invoke(option):
            p=subprocess.run([str(exe),option,str(PRIVATE),text],capture_output=True,text=True,timeout=15)
            require(p.returncode==0,'Diagnostic execution: '+p.stderr)
            return p.stdout.strip()
        wide=invoke('--parse-wide');direct=invoke('--parse-double')
        m=re.fullmatch(r'status=(\d+) consumed=(\d+) converted=(\d+) wide=([0-9a-f]{24}) double=([0-9a-f]+)',wide)
        require(m is not None,'Diagnostic framing')
        require(m[5]==direct.split('=')[-1],'Staged converter versus original atof')
        results.append(dict(input=text,parser_status=int(m[1]),consumed=int(m[2]),converter_status=int(m[3]),wide_hex=m[4],double_hex=m[5]))
    by_text={r['input']:r for r in results}
    require(len({by_text[t]['wide_hex'] for t in neighbors})==1,'Adjacent long decimals collapse before final conversion')
    for f in failures:
        t=bytes.fromhex(f['input_hex']).decode();bits=int(by_text[t]['double_hex'],16)
        d=struct.unpack('<d',struct.pack('<Q',bits))[0]
        if f['query'][2]:
            integer=(int(d)&0xffffffff) if math.isfinite(d) and -2147483648<=d<2147483648 else 0x80000000
        else:
            integer=(int(d)&0xffffffff) if math.isfinite(d) and -(2**63)<=d<2**63 else 0
        require(integer==f['native_words'][1],'Retained constructor failure integer')
    require(sha(LIB)==sha(PRIVATE)==PIN,'Original/private library unchanged')
    protected=ROOT/'work/material_string_value_candidate.h'
    require(sha(protected)=='4a9b16d46bd01e16f034922777e94cfb0d700ea614ab16198ec46c0bf8256007','Verified setter unchanged')
    receipt=dict(status='VERIFIED diagnostic; constructor correction still FAILED/UNVERIFIED',time_utc=datetime.now(timezone.utc).isoformat(),
        executable=str(exe),executable_sha256=sha(exe),library_sha256=PIN,original_constructor_failures_reproduced=len(failures),
        unique_diagnostics=len(results),adjacent_decimal_intermediates_identical=True,static_byte_evidence=evidence,results=results,
        limitations=['No owned conversion correction attempted','No complete constructor acceptance run','No editor build or live SFM access'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(receipt=str(run/'results.json'),diagnostics=len(results),reproduced_failures=len(failures))))
if __name__=='__main__':main()
