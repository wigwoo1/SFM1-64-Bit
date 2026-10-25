"""Verify the installed material-matrix formatted reader, independently of factories."""
from datetime import datetime,timezone
from pathlib import Path
import hashlib,json,random,re,struct,subprocess,tempfile
from fractions import Fraction
from verify_ifm_draw_packet import check_ranges
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha
OUT=ROOT/'outputs/material-matrix-scan-20261025'
def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));require(sha(LIB)==sha(PRIVATE)==PIN,'Library identity')
    im,evidence=check_ranges(LIB,'materialsystem-disassembly.txt',[
        ('sscanf_wrapper',0x1007e535,0x1007e557),
        ('original_numeric_table_initializer',0x1007d4e7,0x1007d547),
        ('formatted_float_dispatch',0x1008320a,0x1008324c),
        ('direct_float_parser',0x1008d565,0x1008d60d),
        ('scanner_scratch_growth',0x10086acb,0x10086b22),
        ('original_calloc',0x10032360,0x1003238f),('original_recalloc',0x100323b0,0x100323e3),
        ('matrix_format_and_count_branch',0x100030e0,0x100030f3),
        ('transform_format_and_count_branch',0x10003142,0x10003155)])
    require(im.string(0x100957b4)==' [ %f %f %f %f  %f %f %f %f  %f %f %f %f  %f %f %f %f ]','Matrix format')
    require(im.string(0x10095780)==' center %f %f scale %f %f rotate %f translate %f %f','Transform format')
    require([im.word(a) for a in range(0x1010510c,0x10105124,4)]==[128,(-127)&0xffffffff,24,8,32,127],'Installed binary32 settings')
    rng=random.Random(2026102507);cases=[];labels=[]
    seed=tuple(0x3f000000+i*0x10000 for i in range(16))
    def add(transform,text,label='controlled',words=seed):
        if isinstance(text,str):text=text.encode()
        cases.append((int(transform),*words,len(text),text));labels.append(label)
    def render(values,transform):
        if not transform:return '[ '+' '.join(values)+' ]'
        return f'center {values[0]} {values[1]} scale {values[2]} {values[3]} rotate {values[4]} translate {values[5]} {values[6]}'
    for transform in [False,True]:
        for text in ['', ' ', '[', '[ ]', 'center','center ', '[x]', 'center x', '[1 2', 'CENTER 1 2 scale 1 1 rotate 0 translate 0 0',
            render(['1']*(7 if transform else 16),transform)]:
            for prefix in ['', ' \t','\n']:
                for suffix in ['', 'tail','\0ignored']:add(transform,prefix+text+suffix,'format_prefix_suffix')
        full=render([str(i) for i in range(7 if transform else 16)],transform)
        for end in range(len(full)+1):add(transform,full[:end],'every_truncation')
        tokens=['0','-0','.5','1e3','1d3','1D-3','1e','1e+','1e-','nan','inf','0x1p2','1.#INF','1,2','1+3','1e309','-1e-999','+','.', '1\0ignored']
        for token in tokens:
            for slot in range(7 if transform else 16):
                v=['1']*(7 if transform else 16);v[slot]=token;add(transform,render(v,transform),'numeric_grammar')
        for _ in range(1500):
            values=[str(rng.randrange(-10**12,10**12))+rng.choice(['e','d','D','E'])+str(rng.randrange(-500,501)) for _ in range(7 if transform else 16)]
            add(transform,render(values,transform),'random_decimal',tuple(rng.getrandbits(32) for _ in range(16)))
    for exponent in [1,2,10,50,100,126,127,128,150,200,253,254]:
        for fraction in [0,2,0x123456,0x7ffffe]:
            bits=(exponent<<23)|fraction
            a=Fraction.from_float(struct.unpack('<f',struct.pack('<I',bits))[0]);b=Fraction.from_float(struct.unpack('<f',struct.pack('<I',bits-1))[0]);mid=(a+b)/2
            power=mid.denominator.bit_length()-1;digits=mid.numerator*(5**power)
            for delta in [-1,0,1]:
                for sign in ['','-']:
                    token=sign+str(digits*10+delta)+'e-'+str(power+1)
                    add(False,render([token]+['1']*15,False),'exact_float_half_neighbors')
    for token in ['1'+('0'*500)+'e-500','0.'+('0'*500)+'1e500','9'*1000+'e-999','-'+'9'*1000+'e-999']:
        add(False,render([token]+['1']*15,False),'long_numeric_tokens')
    for size in range(690,708):
        for sign in ['','+','-']:
            for exponent in ['e0','e+0','e-0']:
                add(False,render([sign+'9'*size+exponent]+['1']*15,False),'scratch_growth_boundaries')
    for token in ['1.000000059604644775390625','1.000000059604644775390626','-1e-999']:
        add(False,render([token]+['1']*15,False),'known_float_bits')
    query=run/'queries.bin';query.write_bytes(b''.join(struct.pack('<18I',*q[:18])+q[18] for q in cases))
    outputs=[];exes=[]
    for tag,suffix,machine in [('native','x86',0x14c),('owned','x64',0x8664)]:
        exe=OUT/f'material_matrix_scan_{tag}_{suffix}.exe';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        require(struct.unpack_from('<H',raw,pe+4)[0]==machine,'PE architecture')
        log=OUT/(tag+'-build.log');require(not re.search(r'\b(warning|error)\b',log.read_text(),re.I),'Clean build '+tag)
        result=run/(tag+'.bin');p=subprocess.run([str(exe),str(PRIVATE),str(query),str(result)],capture_output=True,text=True,timeout=60)
        (run/(tag+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,'Execution '+tag+': '+str(p.returncode)+' '+p.stdout+p.stderr)
        data=result.read_bytes();require(len(data)==68*len(cases),'Output framing');outputs.append(data)
        exes.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),build_log_sha256=sha(log)))
    differences=[dict(case=i,label=labels[i],transform=q[0],input_hex=q[18].hex(),native=outputs[0][68*i:68*i+68].hex(),owned=outputs[1][68*i:68*i+68].hex()) for i,q in enumerate(cases) if outputs[0][68*i:68*i+68]!=outputs[1][68*i:68*i+68]]
    analytical=0
    if not differences:
        def record(transform,text):
            i=next(i for i,q in enumerate(cases) if q[0]==int(transform) and q[18]==text.encode());return struct.unpack_from('<i16I',outputs[1],i*68)
        require(record(False,'')==(-1,*seed),'Empty input preserves output');analytical+=1
        require(record(False,render(['1']*16,False))==(16,*([0x3f800000]*16)),'Full matrix assignments');analytical+=1
        require(record(True,render(['1']*7,True))==(7,*([0x3f800000]*7),*seed[7:]),'Transform assignments preserve tail');analytical+=1
        require(record(False,render(['1.000000059604644775390625']+['1']*15,False))[1]==0x3f800000,'Exact-half down');analytical+=1
        require(record(False,render(['1.000000059604644775390626']+['1']*15,False))[1]==0x3f800000,'Wide intermediate quantizes neighbor to half');analytical+=1
        require(record(False,render(['-1e-999']+['1']*15,False))[1]==0x80000000,'Formatted float signed underflow');analytical+=1
    require(sha(LIB)==sha(PRIVATE)==PIN,'Originals unchanged')
    require(sha(ROOT/'work/material_string_value_candidate.h')=='4a9b16d46bd01e16f034922777e94cfb0d700ea614ab16198ec46c0bf8256007','Protected setter unchanged')
    require(sha(ROOT/'outputs/vtf-legacy-thumbnail-20261024/sfm64_session_ui_vtf_legacy.exe')=='2e79aed74e3831196a609346251680132f7beeadcc0cb810318f1fb9f5156ed2','Editor unchanged')
    receipt=dict(status='FAIL' if differences else 'PASS',utc=datetime.now(timezone.utc).isoformat(),cases=len(cases),differences=differences,exact_bytes=len(outputs[0]),analytical_checks=analytical,executables=exes,
        sources_sha256={name:sha(ROOT/'work'/name) for name in ['material_matrix_scan_candidate.h','material_decimal_float_candidate.h','material_constructor_decimal_candidate.h','probe_material_matrix_scan.cpp','build_material_matrix_scan.cmd','verify_material_matrix_scan.py']},
        query_sha256=sha(query),static_ranges=evidence,library_sha256=PIN,scope='Formatted-reader assignment counts and all16 float32 scratch words; not matrix construction/transform composition or factory execution')
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(receipt=str(run/'results.json'),cases=len(cases),differences=len(differences),first=differences[:3])))
    require(not differences,'Matrix formatted reader parity')
if __name__=='__main__':main()
