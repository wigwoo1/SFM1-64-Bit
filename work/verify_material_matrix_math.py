"""Private matrix arithmetic comparison. Complete material loader remains separate."""
from datetime import datetime,timezone
from pathlib import Path
from collections import Counter
import hashlib,json,math,random,re,struct,subprocess,tempfile
from verify_ifm_draw_packet import check_ranges
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha
OUT=ROOT/'outputs/material-matrix-math-20261025'
def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]
def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));require(sha(LIB)==sha(PRIVATE)==PIN,'Library identity')
    im,evidence=check_ranges(LIB,'materialsystem-disassembly.txt',[
        ('complete_matrix_multiply',0x1006cb00,0x1006cf01),('complete_translate',0x1006d040,0x1006d0b1),
        ('complete_scale',0x1006d240,0x1006d2b1),('complete_rotation',0x1006d190,0x1006d231),
        ('sine_fast_tiny_and_fallback_dispatch',0x10081314,0x10081463),
        ('cosine_fast_tiny_and_fallback_dispatch',0x10081786,0x100818dd),
        ('double_sine_cpu_dispatch_and_x87_path',0x1008b380,0x1008b4a9),
        ('double_cosine_cpu_dispatch_and_x87_path',0x1008ba10,0x1008bb39),
        ('double_sine_sse2_and_x87_fallback_jump',0x10090f78,0x10091127),
        ('double_cosine_sse2_and_x87_fallback_jump',0x10091148,0x100912d9)])
    offset=im.offset(0x100de848);require(im.b[offset:offset+8]==struct.pack('<d',0.017453292519943295),'Degree conversion constant')
    rng=random.Random(2026102508);cases=[];labels=[]
    identity=tuple(0x3f800000 if i%5==0 else 0 for i in range(16))
    def add(op,a,b=identity,alias=0,label='controlled'):
        a=tuple(a)+(0,)*(16-len(a));cases.append((op,alias,*a,*b));labels.append(label)
    for index in range(16):
        a=[0]*16;a[index]=bits(index+1)
        for alias in range(4):add(0,a,identity,alias,'identity_basis')
    for _ in range(2000):
        a=tuple(bits(rng.uniform(-1000,1000)) for _ in range(16));b=tuple(bits(rng.uniform(-1000,1000)) for _ in range(16))
        for alias in range(4):add(0,a,b,alias,'finite_multiply')
    special=[0,0x80000000,1,0x80000001,0x7fffff,0x800000,0x3f800000,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f800001]
    for _ in range(512):
        a=tuple(rng.choice(special) for _ in range(16));b=tuple(rng.choice(special) for _ in range(16))
        for alias in range(4):add(0,a,b,alias,'special_multiply')
    for op in [1,2]:
        for a in [(x,y,z) for x in special[:8] for y in special[:8] for z in [0,0x80000000,0x3f800000]]:add(op,a,label='translate_scale')
    angles=[0.,-0.,1.,-1.,90.,180.,270.,360.,-90.,0.00001,359.9999]+[rng.uniform(-36000,36000) for _ in range(4096)]
    for angle in angles:add(3,[bits(angle)],label='rotation')
    for _ in range(2048):
        p=[bits(rng.uniform(-10,10)) for _ in range(7)];p[4]=bits(rng.uniform(-3600,3600));add(4,p,label='composed_transform')
    baseline_bytes=b''.join(struct.pack('<34I',*q) for q in cases)
    baseline_sha=hashlib.sha256(baseline_bytes).hexdigest()
    require(len(cases)==16651 and baseline_sha=='6ec4a8244f01a09ba15f18725d769eb84904b18ab46e350ae8e374883e51c2e6','Original corpus must remain unchanged')
    for delta in range(-512,513):
        for sign in [0,0x80000000]:add(3,[(0x4658f683+delta)^sign],label='failing_degree_neighborhood')
    focused=random.Random(2026102601)
    for _ in range(25000):add(5,[focused.randrange(0x46000000)|(focused.randrange(2)<<31)],label='fast_trig_random_bits')
    # Actual nearest-integer selection transitions, on both signs and every
    # table phase, plus large fast-range indices near the fallback boundary.
    for n in list(range(-512,513))+list(range(83390,83445))+list(range(-83445,-83390)):
        center=bits((n+0.5)*math.pi/32)
        for delta in range(-2,3):add(5,[center+delta],label='trig_table_transition')
    for center in [0,1,0x007fffff,0x00800000,0x43725995,0x46000000]:
        for delta in range(-8,9):
            if 0<=center+delta<0x7f800000:
                for sign in [0,0x80000000]:add(5,[(center+delta)^sign],label='trig_path_boundary')
    for value in [0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f800001,0xff800001]:add(5,[value],label='trig_nonfinite')
    fallback_inputs=[focused.randrange(0x46000000,0x7f800000)|(focused.randrange(2)<<31) for _ in range(256)]
    for value in fallback_inputs:add(5,[value],label='unverified_large_fallback_diagnostic')
    for value in fallback_inputs:add(5,[value],alias=1,label='unverified_large_sse2_fallback_diagnostic')
    prior_bytes=b''.join(struct.pack('<34I',*q) for q in cases)
    prior_sha=hashlib.sha256(prior_bytes).hexdigest()
    require(len(cases)==50068 and prior_sha=='9d7432a98e64e7ff0eafa6e6d5d32c258d5ff5666b485f9c18e6caa2e2f06278','Expanded failing corpus must remain unchanged')
    large=random.Random(2026102602)
    for _ in range(32768):
        value=large.randrange(0x46000000,0x7f800000)|(large.randrange(2)<<31)
        for mode in [0,1]:add(5,[value],alias=mode,label='large_radian_bits')
    for _ in range(20000):
        value=bits(large.uniform(8192,90112))^(large.randrange(2)<<31)
        for mode in [0,1]:add(5,[value],alias=mode,label='moderate_double_dispatch')
    for _ in range(1024):
        n=large.randrange(83444,917800)
        center=bits((n+.5)*math.pi/32)
        for delta in range(-2,3):
            for sign in [0,0x80000000]:
                for mode in [0,1]:add(5,[(center+delta)^sign],alias=mode,label='double_table_transition')
    for center in [0x46000000,0x47b00000,0x5f000000,0x5f490fdb,0x7f7fffff]:
        for delta in range(-16,17):
            if 0<=center+delta<0x7f800000:
                for sign in [0,0x80000000]:
                    for mode in [0,1]:add(5,[(center+delta)^sign],alias=mode,label='large_dispatch_boundary')
    degree_inputs=[large.getrandbits(32) for _ in range(8192)]+special+[x^0x80000000 for x in special]
    for value in degree_inputs:
        for mode in [0,1]:add(3,[value],alias=mode,label='whole_rotation_bits')
    for _ in range(256):
        p=[bits(large.uniform(-10,10)) for _ in range(7)];p[4]=large.getrandbits(32)
        for mode in [0,1]:add(4,p,alias=mode,label='large_composed_transform')
    add(6,[],label='independent_extended_period')
    query=run/'queries.bin';query.write_bytes(b''.join(struct.pack('<34I',*q) for q in cases));outputs=[];exes=[]
    for tag,suffix,machine in [('native','x86',0x14c),('owned','x64',0x8664)]:
        exe=OUT/f'material_matrix_math_{tag}_{suffix}.exe';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(struct.unpack_from('<H',raw,pe+4)[0]==machine,'Architecture')
        log=OUT/(tag+'-build.log');require(not re.search(r'\b(warning|error)\b',log.read_text(),re.I),'Clean build '+tag)
        if tag=='owned':require(not re.search(r'\b(warning|error)\b',(OUT/'asm-build.log').read_text(),re.I),'Clean owned assembler build')
        library_arg=PRIVATE if tag=='native' else run/'no-engine-present.dll'
        if tag=='owned':require(not library_arg.exists(),'Owned must run without original-library argument')
        result=run/(tag+'.bin');p=subprocess.run([str(exe),str(library_arg),str(query),str(result)],capture_output=True,text=True,timeout=60)
        (run/(tag+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,'Execution '+tag+': '+str(p.returncode)+' '+p.stdout+p.stderr)
        data=result.read_bytes();require(len(data)==64*len(cases),'Output framing');outputs.append(data);exes.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),build_log_sha256=sha(log),library_argument=str(library_arg)))
    # Independently derived extended value: correctly rounded pi*2^62 has a
    # 64-bit significand and exponent63 (biased0x403e). The decimal recovery
    # independently regenerates this mathematical constant at110 digits.
    expected_period=struct.pack('<QH',14488038916154245685,0x403e)+bytes(54)
    require(outputs[0][-64:]==outputs[1][-64:]==expected_period,'Assembled extended period bits')
    differences=[dict(case=i,label=labels[i],query=q,native=outputs[0][64*i:64*i+64].hex(),owned=outputs[1][64*i:64*i+64].hex()) for i,q in enumerate(cases) if outputs[0][64*i:64*i+64]!=outputs[1][64*i:64*i+64]]
    require(sha(LIB)==sha(PRIVATE)==PIN,'Original library changed')
    receipt=dict(status='FAIL' if differences else 'PASS',utc=datetime.now(timezone.utc).isoformat(),cases=len(cases),groups=dict(Counter(labels)),differences=differences,exact_bytes=len(outputs[0]),executables=exes,static_ranges=evidence,library_sha256=PIN,
        original_cases=16651,original_query_sha256=baseline_sha,query_sha256=sha(query),
        previous_expanded_cases=50068,previous_expanded_query_sha256=prior_sha,
        previous_expanded_differences=sum(d['case']<50068 for d in differences),
        owned_no_original_library_argument=True,owned_x87_control_and_stack_top_checked_per_case=True,
        owned_assembly_build_log_sha256=sha(OUT/'asm-build.log'),
        original_differences=sum(d['case']<16651 for d in differences),
        differences_by_group=dict(Counter(d['label'] for d in differences)),
        scope='Row-major arithmetic, original rotation wrapper, direct trig and composed primitive transforms; full material parser/factory not yet executed',
        limits=['Round-to-nearest-even, gradual underflow; no FP exception/status contract',
                'Owned x87 FSIN/FCOS/FPREM1 with pi*2^62 period and reconstructed moderate SSE2 path; tested on the recorded host CPU, not an exhaustive all-input or all-CPU proof',
                'Owned assembly checks preserve control word and x87 stack TOP per query, not FP status flags',
                'Original no-loader default CPU flag0 selects x87 fallback; flag1 selects SSE2 under fixture controls. Live SFM dispatch is UNVERIFIED',
                'Private x86 reference is verification-only, never an owned x64 runtime dependency',
                'No production integration or editor build/run'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(receipt=str(run/'results.json'),cases=len(cases),differences=len(differences),groups=dict(Counter(d['label'] for d in differences)),first=differences[:2])))
    require(not differences,'Matrix arithmetic parity')
if __name__=='__main__':main()
