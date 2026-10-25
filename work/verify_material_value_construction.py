"""Verify installed typed constructors, not a full live VMT/material load."""
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path
import hashlib,json,random,re,struct,subprocess,tempfile,zlib
from fractions import Fraction
from verify_ifm_draw_packet import check_ranges
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha

OUT=ROOT/'outputs/material-value-construction-20261025'
NAMES=['integer','float','vector','string','undefined']


def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
    print('Verification directory: '+str(run),flush=True)
    require(sha(LIB)==sha(PRIVATE)==PIN,'Library identity')
    image,evidence=check_ranges(LIB,'materialsystem-disassembly.txt',[
        ('complete_string_constructor',0x10017780,0x10017842),
        ('complete_vector_constructor',0x10017850,0x1001790f),
        ('complete_float_constructor',0x10017910,0x100179a1),
        ('complete_integer_constructor',0x100179b0,0x10017a4c),
        ('complete_undefined_constructor',0x10017a50,0x10017acb),
        ('complete_null_name_intern_branch',0x10017570,0x1001759d),
        ('both_string_double_to_integer_paths',0x1007dc10,0x1007dcbb),
        ('complete_KeyValues_creation_dispatch_STATIC_ONLY',0x100032a0,0x10003384),
        ('string_creation_factory_STATIC_ONLY',0x10018c60,0x10018c8c),
        ('vector_creation_factory_STATIC_ONLY',0x10018c90,0x10018cc0),
        ('float_creation_factory_STATIC_ONLY',0x10018cc0,0x10018cf3),
        ('integer_creation_factory_STATIC_ONLY',0x10018d00,0x10018d2c),
        ('undefined_creation_factory_STATIC_ONLY',0x10018d30,0x10018d58),
        ('creation_dispatch_caller_STATIC_ONLY',0x10005c1c,0x10005c29),
        ('KeyValues_type_accessor_STATIC_ONLY',0x10071260,0x10071280),
        ('legacy_double_round_guard_and_sticky',0x1008e8eb,0x1008e94c)])
    require(image.word(0x101050fc)==53,'Installed decimal conversion precision')
    exes=[]
    for tag,machine,magic in [('native',0x14c,0x10b),('owned',0x8664,0x20b)]:
        p=OUT/('material_construction_'+tag+('_x86.exe' if tag=='native' else '_x64.exe'))
        raw=p.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==magic,'Architecture '+tag)
        log=OUT/(tag+'-build.log');require(not re.search(r'\b(?:warning|error)\b',log.read_text(),re.I),'Build diagnostics '+tag)
        exes.append(dict(path=str(p),sha256=sha(p),machine=hex(machine),build_log_sha256=sha(log)))
    p=subprocess.run([exes[1]['path'],'--test'],capture_output=True,text=True,timeout=30)
    (run/'analytical.log').write_text(p.stdout+p.stderr)
    require(p.returncode==0 and 'checks=15 refusal_cases=6' in p.stdout,'Analytical checks')
    rows=[];labels=[];rng=random.Random(2026102521)
    def add(kind,args=(0,0,0,0),count=0,text=b'',sse2=1,fill=0,label='controlled'):
        if isinstance(text,str):text=text.encode()
        rows.append((kind,count,sse2,fill,*args,len(text),text));labels.append(label)
    strings=['','1e3','-1e3','16777217','-16777217','1.9999999999999999999','2147483647','2147483648',
        '-2147483649','4294967295','4294967296','-4294967296','9223372036854774784','9223372036854775808',
        '-9223372036854775808','1e309','-1e309','1e-999','-1e-999','12.5suffix','textures/pyro','nan','inf','0x1p2','3\0ignored']
    special=[0,0x80000000,1,0x80000001,0x7fffff,0x800000,0x3f800000,0x3fffffff,0x4b800001,
        0x4effffff,0x4f000000,0xcf000000,0xcf000001,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f800001]
    for fill in range(256):
        for sse2 in range(2):
            add(4,sse2=sse2,fill=fill,label='all_initial_bytes')
            add(0,args=(rng.getrandbits(32),0,0,0),sse2=sse2,fill=fill,label='all_initial_bytes')
            add(1,args=(special[fill%len(special)],0,0,0),sse2=sse2,fill=fill,label='all_initial_bytes')
            for n in range(1,5):
                add(2,args=tuple(special[(fill+k)%len(special)] for k in range(4)),count=n,sse2=sse2,fill=fill,label='all_initial_bytes')
            add(3,text=strings[fill%len(strings)],sse2=sse2,fill=fill,label='all_initial_bytes')
    for s in strings:
        for mode in range(2):add(3,text=s,sse2=mode,label='constructor_setter_and_cpu_distinctions')
    # Exact binary64 halfway values and neighbors spanning subnormal, ordinary
    # and large magnitudes. Fraction construction avoids a modern parser oracle.
    for exponent in [1,2,20,100,500,900,1000,1022,1023,1024,1040,1054,1070,1500,2000,2046]:
        for fraction in [0,2,4,0x123456789abce,0xffffffffffffe]:
            bits=(exponent<<52)|fraction
            d=struct.unpack('<d',struct.pack('<Q',bits))[0]
            previous=struct.unpack('<d',struct.pack('<Q',bits-1))[0]
            midpoint=(Fraction.from_float(d)+Fraction.from_float(previous))/2
            power=midpoint.denominator.bit_length()-1;coefficient=midpoint.numerator*5**power
            for delta in [-1,0,1]:
                decimal=str(coefficient*10+delta)+'e-'+str(power+1)
                for sign in ['', '-']:
                    for mode in range(2):add(3,text=sign+decimal,sse2=mode,label='exact_double_halfway_and_decimal_neighbors')
    for _ in range(4096):
        kind=rng.randrange(3)
        add(kind,args=tuple(rng.getrandbits(32) for _ in range(4)),count=rng.randrange(1,5) if kind==2 else 0,
            fill=rng.randrange(256),sse2=rng.randrange(2),label='random_numeric_bits')
    for _ in range(2048):
        digits=''.join(str(rng.randrange(10)) for _ in range(rng.randrange(1,36)))
        split=rng.randrange(len(digits)+1)
        text=rng.choice(['',' ','\t'])+rng.choice(['','+','-'])+digits[:split]+'.'+digits[split:]+rng.choice(['','e'+str(rng.randrange(-330,331)),'suffix'])
        for mode in range(2):add(3,text=text,sse2=mode,label='random_decimal_both_cpu_paths')
    prior=json.loads((ROOT/'outputs/material-string-value-20261025/verification-zkqa42up/results.json').read_text())
    authored=prior['authored_material_tokens']
    for m in authored:
        require(sha(m['path'])==m['sha256'],'VMT identity')
        for token in re.findall(rb'"([^"\r\n]*)"',Path(m['path']).read_bytes()):
            for mode in range(2):add(3,text=token,sse2=mode,label='benchmark_literal_tokens_not_effective_materials')
    baseline=b''.join(struct.pack('<9I',*row[:9])+row[9] for row in rows)
    require(len(rows)==13698 and baseline==(OUT/'verification-cibel3w6/queries.bin').read_bytes(),'Complete original constructor corpus preserved')
    extras=['1d3','-1D-3','1e','1e+','1e-','1+3','1-3','\v1.5','\f-1.5','-0','1\0e9']
    extras += ['12345678901234567890123'+str(a)+str(b)+'e-24' for a in range(10) for b in range(10)]
    for s in extras:
        for mode in range(2):add(3,text=s,sse2=mode,label='reconstructed_parser_edges')
    query=run/'queries.bin';query.write_bytes(b''.join(struct.pack('<9I',*row[:9])+row[9] for row in rows))
    outputs=[];logs=[]
    for tag,exe in zip(['native','owned'],exes):
        target=run/(tag+'.bin');p=subprocess.run([exe['path'],str(PRIVATE),str(query),str(target)],capture_output=True,text=True,timeout=120)
        (run/(tag+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0 and 'PASS material_construction_cases='+str(len(rows)) in p.stdout,tag+' exit='+str(p.returncode)+' '+p.stdout+p.stderr)
        outputs.append(target.read_bytes());logs.append(p.stdout.strip())
    def decode(raw):
        at=0;records=[]
        for _ in rows:
            require(at+28<=len(raw),'Output framing');h=struct.unpack_from('<7I',raw,at);size=28+h[6]
            require(at+size<=len(raw),'Output text framing');records.append((h,raw[at+28:at+size],raw[at:at+size]));at+=size
        require(at==len(raw),'Trailing output');return records
    native,owned=map(decode,outputs)
    differences=[dict(case=i,label=labels[i],query=rows[i][:9],input_hex=rows[i][9].hex(),native=n[:2],owned=o[:2]) for i,(n,o) in enumerate(zip(native,owned)) if n[2]!=o[2]]
    for d in differences:d['native']=(d['native'][0],d['native'][1].hex());d['owned']=(d['owned'][0],d['owned'][1].hex())
    (run/'comparison.json').write_text(json.dumps(dict(differences=len(differences),details=differences[:8]),indent=2)+'\n')
    require(not differences,'Constructor comparison '+json.dumps(differences[:3]))
    for row,(h,text,_) in zip(rows,native):
        expected_meta=0xffffff00|((row[1]<<4|2) if row[0]==2 else [0x44,0x40,0,0x41,0x46][row[0]])
        require(h[0]==expected_meta,'Constructor metadata')
        require(text==(row[9].split(b'\0',1)[0] if row[0]==3 else b''),'Constructor active string')
    # Prior verified string implementation must remain independently reproducible.
    python='C:/Users/wigwo/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
    regression=subprocess.run([python,str(ROOT/'work/verify_material_string_value.py')],capture_output=True,text=True,timeout=120)
    (run/'string-regression.log').write_text(regression.stdout+regression.stderr);require(regression.returncode==0,'String regression '+regression.stderr)
    summary=json.loads(regression.stdout.strip().splitlines()[-1]);regpath=Path(summary['receipt']);reg=json.loads(regpath.read_text())
    require(reg['status']=='PASS' and reg['query_sha256']==prior['query_sha256'] and reg['native_output_sha256']==prior['native_output_sha256']==reg['owned_output_sha256'],'Previous full string corpus changed')
    previous_path=ROOT/'outputs/material-string-value-20261025/completion-check.json';previous=json.loads(previous_path.read_text())
    for name,pin in previous['sources_sha256'].items():
        if name!='material_string_value_candidate.h':require(sha(ROOT/'work'/name)==pin,'Previous source changed '+name)
    require(sha(previous['editor']['path'])==previous['editor']['sha256'],'Editor changed')
    require(sha(LIB)==sha(PRIVATE)==PIN and all(sha(m['path'])==m['sha256'] for m in authored),'Originals changed')
    archive=run/'native-construction-records.bin.zlib';archive.write_bytes(zlib.compress(outputs[0],9));require(zlib.decompress(archive.read_bytes())==outputs[0]==outputs[1],'Exact archive')
    outputsha=sha(run/'native.bin')
    for tag in ['native','owned']:(run/(tag+'.bin')).unlink()
    tracked=['material_value_construction_candidate.h','material_constructor_decimal_candidate.h','material_decimal_digits_candidate.h','material_decimal_powers_candidate.h','material_decimal_scaling_candidate.h','material_decimal_double_candidate.h','material_construction_native_fixture.h','probe_material_value_construction.cpp','build_material_value_construction.cmd','verify_material_value_construction.py','material_string_value_candidate.h']
    r=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),cases=len(rows),exact_value_text_bytes=len(outputs[0]),differences=0,analytical_checks=15,refusal_cases=6,
        constructors=dict(Counter(NAMES[row[0]] for row in rows)),cpu_paths=dict(Counter(str(row[2]) for row in rows)),corpus_groups=dict(Counter(labels)),executables=exes,logs=logs,
        string_regression=dict(receipt=str(regpath),receipt_sha256=sha(regpath),cases=reg['cases'],steps=reg['steps'],old_queries_and_outputs_unchanged=True),
        static_ranges=evidence,static_instruction_bytes=sum(x['bytes'] for x in evidence),
        sources_sha256={n:sha(ROOT/'work'/n) for n in tracked},query_sha256=sha(query),output_sha256=outputsha,
        exact_output_archive=dict(path=str(archive),sha256=sha(archive),bytes=archive.stat().st_size,decompressed_bytes=len(outputs[0]),roundtrip=True),
        original_materialsystem_sha256=PIN,originals_unchanged=True,authored_material_tokens=authored,editor=previous['editor'],
        construction_dispatch_static=dict(entry='0x100032a0',caller='0x10005c1c',KeyValues_string=1,KeyValues_integer=2,KeyValues_float=3,
            string_order=['empty rejected','attempt matrix parse','bracket/brace vector or string'],scope='BYTE_CHECKED ONLY; full KeyValues factory not executed'),
        computer_use=False,sfm_process_used=False,dll_loader_used=False,gpu_used=False,
        limitations=['Actual complete constructors execute with null owner/name, original C-locale parsing and both controlled CRT CPU paths.',
            'Name interning, pooled factory allocation, complete KeyValues dispatch/matrix/vector parsing and effective material typing are not yet executed here.',
            'CPU feature byte1013FF3C is controlled0/1, not a claim about live engine selection. Owned mode explicitly chooses the conversion contract.',
            'Normalized values include no-name/temporary metadata but not original pointer/owner/resource identities. Owned storage uses RAII.',
            'String constructor truncates parsed double; setter uses saturated integer-prefix parsing. Float32 cache must not be reused for constructor integer.',
            'Other locale/codepage/FP status, texture resources, initializer composition, production integration and full shading remain unverified.',
            'Production source dependencies unchanged; no complete editor build/run.'])
    (run/'results.json').write_text(json.dumps(r,indent=2)+'\n')
    print(json.dumps(dict(status='PASS',receipt=str(run/'results.json'),cases=len(rows),exact_bytes=r['exact_value_text_bytes'],analytical_checks=15,refusal_cases=6,string_regression_steps=reg['steps'])),flush=True)


if __name__=='__main__':main()
